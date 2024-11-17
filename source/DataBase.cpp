#include "DataBase.h"
#include "ThreadPool.h"

DataBase::DataBase(const std::string &connectionString)
    : m_connection_string(connectionString)
{
    mongoc_init();

    m_client = mongoc_client_new(connectionString.c_str());
    if (!m_client)
    {
        std::cerr << "Failed to create a new client instance." << std::endl;
        return;
    }

    bson_t *command = BCON_NEW("ping", BCON_INT32(1));
    bson_error_t error;
    bool retval = mongoc_client_command_simple(m_client, "admin", command, nullptr, nullptr, &error);

    if (!retval)
    {
        std::cerr << "Failed to run command: " << error.message << std::endl;
    }
    else
    {
        std::cout << "Connected successfully." << std::endl;
    }

    bson_destroy(command);
}

void DataBase::processWordDocuments(std::vector<bson_t *> &documents, const std::string &term, const std::unordered_map<std::string, std::vector<int>> &map)
{
    try
    {
        if (term.empty())
        {
            return;
        }
        bson_t *term_doc = bson_new();
        if (!term_doc)
        {
            std::cerr << "Failed to create BSON document." << std::endl;
        }
        bson_t postings_array;
        bson_append_utf8(term_doc, "term", -1, term.c_str(), -1);
        BSON_APPEND_ARRAY_BEGIN(term_doc, "documents", &postings_array);
        // BSON_APPEND_ARRAY_BEGIN(document, term.c_str(), &term_doc);
        int i = 0;
        for (const auto &[docId, positions] : map)
        {
            if (docId.empty() || positions.empty())
                continue;
            bson_t posting_doc{};
            std::string key = std::to_string(i++);
            BSON_APPEND_DOCUMENT_BEGIN(&postings_array, key.c_str(), &posting_doc);

            BSON_APPEND_UTF8(&posting_doc, "docId", docId.c_str());

            bson_t positions_array{};
            int pos_index{};
            BSON_APPEND_ARRAY_BEGIN(&posting_doc, "positions", &positions_array);

            for (int pos : positions)
            {
                std::string key = std::to_string(pos_index); // Use pos_index without incrementing
                BSON_APPEND_INT32(&positions_array, key.c_str(), pos);
                pos_index++;
            }
            bson_append_array_end(&posting_doc, &positions_array);

            bson_append_document_end(&postings_array, &posting_doc);
        }
        // bson_append_array_end(document, &term_doc);
        bson_append_array_end(term_doc, &postings_array);
        if (term_doc != nullptr)
        {
            documents.push_back(term_doc);
        }
        else
        {
            std::cerr << "Didn't insert the faulty document\n";
        }
    }
    catch (std::exception &ex)
    {
        std::cerr << ex.what();
    }
}

DataBase::~DataBase()
{

    if (m_client)
    {
        mongoc_client_destroy(m_client);
    }
}

void DataBase::insertDocument(const bson_t *document, const std::string &database_name, const std::string &collection_name)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    mongoc_collection_t *collection = nullptr;
    bson_error_t error;
    bool insertSuccess = false;
    try
    {
        collection = mongoc_client_get_collection(m_client, database_name.c_str(), collection_name.c_str());
        if (!collection)
        {
            std::cerr << "Failed to get collection: " << collection_name << std::endl;
            return;
        }

        std::cout << "Collection obtained successfully." << std::endl;

        insertSuccess = mongoc_collection_insert_one(collection, document, nullptr, nullptr, &error);
        if (!insertSuccess)
        {
            std::cerr << "Failed to insert document: " << error.message << std::endl;
        }
        else
        {
            std::cout << "Document inserted successfully." << std::endl;
        }
    }
    catch (std::exception &ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }

    if (collection)
    {
        mongoc_collection_destroy(collection);
    }
}
void DataBase::insertManyDocuments(std::vector<bson_t *> &documents, const std::string &database_name, const std::string &collection_name)
{
    std::lock_guard<std::mutex> lock(dbMutex);

    if (documents.size() == 0)
    {
        std::cout << "No documents to insert\n";
        return;
    }
    mongoc_collection_t *collection = nullptr;
    bson_error_t error;
    bool insertSuccess = false;
    try
    {
        collection = mongoc_client_get_collection(m_client, database_name.c_str(), collection_name.c_str());
        if (!collection)
        {
            std::cerr << "Failed to get collection: " << collection_name << std::endl;
            return;
        }

        std::cout << "Collection obtained successfully." << std::endl;
        std::vector<const bson_t *> documentPointers(documents.begin(), documents.end());

        // for (const bson_t *doc : documentPointers)
        // {
        //     if (doc == nullptr)
        //     {
        //         std::cout << "Found a null document pointer in documentPointers\n";
        //         // Handle error
        //     }
        // }
        insertSuccess = mongoc_collection_insert_many(collection, documentPointers.data(), documentPointers.size(), nullptr, nullptr, &error);
        if (!insertSuccess)
        {
            std::cerr << "Failed to insert documents: " << error.message << std::endl;
        }
        else
        {
            std::cout << "Documents inserted successfully." << std::endl;
        }
        for (bson_t *document : documents)
        {
            bson_destroy(document);
        }
        documents.clear();
    }
    catch (std::exception &ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }

    if (collection)
    {
        mongoc_collection_destroy(collection);
    }
}
void DataBase::insertOrUpdateManyDocuments(std::vector<std::pair<bson_t *, bson_t *>> &filterAndUpdateDocuments, const std::string &database_name, const std::string &collection_name)
{
    // std::lock_guard<std::mutex> lock(dbMutex);
    if (filterAndUpdateDocuments.empty())
    {
        std::cout << "No documents to insert or update\n";
        return;
    }

    bson_t *opts;
    opts = bson_new();
    BSON_APPEND_BOOL(opts, "ordered", false);

    mongoc_collection_t *collection = nullptr;
    bson_error_t error;
    try
    {
        collection = mongoc_client_get_collection(m_client, database_name.c_str(), collection_name.c_str());
        if (!collection)
        {
            std::cerr << "Failed to get collection: " << collection_name << std::endl;
            return;
        }
        std::cout << "Collection obtained successfully." << std::endl;
        mongoc_bulk_operation_t *bulk = mongoc_collection_create_bulk_operation_with_opts(collection, opts);
        for (auto [filter, update] : filterAndUpdateDocuments)
        {
            mongoc_bulk_operation_update_one_with_opts(bulk, filter, update, BCON_NEW("upsert", BCON_BOOL(true)), NULL);
        }
        if (!mongoc_bulk_operation_execute(bulk, NULL, &error))
        {
            std::cerr << "Failed to insert/update documents: " << error.message << std::endl;
        }
        else
        {
            std::cout << "Documents inserted/updated successfully." << std::endl;
        }
        for (auto [filter, update] : filterAndUpdateDocuments)
        {
            bson_destroy(filter);
            bson_destroy(update);
        }
        filterAndUpdateDocuments.clear();
    }
    catch (std::exception &ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }

    if (collection)
    {
        mongoc_collection_destroy(collection);
    }
}
bson_t *DataBase::getDocument(const std::string &database_name, const std::string &collection_name)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    bson_t *document = nullptr;
    mongoc_collection_t *collection = nullptr;
    mongoc_cursor_t *cursor = nullptr;
    const bson_t *result = nullptr;

    try
    {
        collection = mongoc_client_get_collection(m_client, database_name.c_str(), collection_name.c_str());
        if (!collection)
        {
            std::cerr << "Failed to get collection: " << collection_name << std::endl;
            return {};
        }
        bson_t query;
        bson_init(&query);

        cursor = mongoc_collection_find_with_opts(collection, &query, nullptr, nullptr);
        if (mongoc_cursor_next(cursor, &result))
        {
            document = bson_copy(result);
        }
        else
        {
            std::cerr << "No document found in the collection: " << collection_name << std::endl;
        }
    }
    catch (...)
    {
        std::cerr << "An error ocurred while getting the document." << std::endl;
    }
    if (collection)
    {
        mongoc_collection_destroy(collection);
    }
    if (cursor)
    {
        mongoc_cursor_destroy(cursor);
    }
    return document ? document : nullptr;
}
std::vector<bson_t *> DataBase::getAllDocuments(const std::string &database_name, const std::string &collection_name, bson_t *filters)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    std::vector<bson_t *> documents{};
    mongoc_collection_t *collection = nullptr;
    mongoc_cursor_t *cursor = nullptr;
    try
    {
        collection = mongoc_client_get_collection(m_client, database_name.c_str(), collection_name.c_str());
        if (!collection)
        {
            std::cerr << "Failed to get collection: " << collection_name << std::endl;
            return {};
        }
        cursor = mongoc_collection_find_with_opts(collection, filters, nullptr, nullptr);
        if (!cursor)
        {
            std::cerr << "Failed to get cursor to collection: " << collection_name << std::endl;
            return {};
        }

        const bson_t *doc;
        while (mongoc_cursor_next(cursor, &doc))
        {
            bson_t *copied_doc = bson_copy(doc);
            documents.push_back(copied_doc);
        }
    }
    catch (std::exception &ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
    if (cursor != nullptr)
        mongoc_cursor_destroy(cursor);
    if (collection != nullptr)
        mongoc_collection_destroy(collection);

    return documents;
}

std::vector<bson_t *> DataBase::getLimitedDocuments(const std::string &database_name, const std::string &collection_name, int limit, bson_t *filters)
{
    bson_t *opts = bson_new();
    BSON_APPEND_INT64(opts, "limit", limit);
    mongoc_collection_t *collection = nullptr;
    bson_error_t error;
    try
    {
        collection = mongoc_client_get_collection(m_client, database_name.c_str(), collection_name.c_str());
        if (!collection)
        {
            std::cerr << "Failed to get collection: " << collection_name << std::endl;
            return std::vector<bson_t *>{};
        }
        mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, filters, opts, NULL);
        std::vector<bson_t *> documents;
        const bson_t *doc;
        bson_error_t error;
        while (mongoc_cursor_next(cursor, &doc))
        {
            bson_t *copy = bson_copy(doc);
            documents.push_back(copy);
        }
        if (mongoc_cursor_error(cursor, &error))
        {
            std::cerr << "Cursor error: " << error.message << "\n";
        }
        bson_destroy(filters);
        bson_destroy(opts);
        mongoc_cursor_destroy(cursor);
        mongoc_collection_destroy(collection);
        return documents;
    }
    catch (std::exception &ex)
    {
        std::cerr << ex.what();
    }
}

int DataBase::getCollectionDocumentCount(const std::string &database_name, const std::string &collection_name, bson_t *filter)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    mongoc_collection_t *collection;
    int count{};
    try
    {
        collection = mongoc_client_get_collection(m_client, database_name.c_str(), collection_name.c_str());
        if (!collection)
        {
            std::cerr << "Failed to get collection: " << collection_name << "\n";
            return -1;
        }
        bson_error_t error;
        count = mongoc_collection_count_documents(collection, filter, NULL, NULL, NULL, &error);
        if (collection)
        {
            mongoc_collection_destroy(collection);
        }
        if (count >= 0)
        {
            std::cout << "Document count: " << count << '\n';
        }
        else
        {
            std::cerr << "Error counting documents: " << error.message << "\n";
            return -1;
        }
    }
    catch (std::exception &ex)
    {
        std::cerr << ex.what();
    }

    return count;
}

std::vector<bson_t *> DataBase::getDocumentsByIds(const std::string &database_name, const std::string &collection_name, const std::vector<std::string> &ids)
{
    bson_t *filter = DataBase::createIdFilter(ids);
    return DataBase::getLimitedDocuments(database_name, collection_name, 100, filter);
}

bson_t *DataBase::createIdFilter(const std::vector<std::string> &ids)
{
    bson_t *filter = bson_new();
    bson_t or_array;
    bson_t child;
    bson_append_array_begin(filter, "$or", -1, &or_array);
    for (size_t i = 0; i < ids.size(); ++i)
    {
        std::string key = std::to_string(i);
        BSON_APPEND_DOCUMENT_BEGIN(&or_array, key.c_str(), &child);
        bson_oid_t id;
        bson_oid_init_from_string(&id, ids[i].c_str());

        BSON_APPEND_OID(&child, "_id", &id);
        bson_append_document_end(&or_array, &child);
    }
    bson_append_array_end(filter, &or_array);
    return filter;
}

void DataBase::clearCollection(const std::string &database_name, const std::string collection_name)
{

    mongoc_collection_t *collection = nullptr;
    try
    {
        std::lock_guard<std::mutex> lock(dbMutex);
        collection = mongoc_client_get_collection(m_client, database_name.c_str(), collection_name.c_str());
        if (!collection)
        {
            throw std::runtime_error("Failed to get collection.");
        }

        bson_t *filter = bson_new();

        bson_error_t error;
        if (!mongoc_collection_delete_many(collection, filter, nullptr, nullptr, &error))
        {
            throw std::runtime_error(std::string("Failed to delete documents: ") + error.message);
        }
        bson_destroy(filter);
    }
    catch (std::exception &ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
    if (collection)
    {
        mongoc_collection_destroy(collection);
    }
}

void DataBase::saveInvertedIndex(const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> &index, const std::string &database_name, const std::string collection_name)
{

    // bson_t *document = bson_new();
    std::vector<bson_t *> documents{};
    try
    {

        for (const auto &[term, map] : index)
        {
            processWordDocuments(documents, term, map);
        }

        // insertDocument(document, database_name,collection_name);
        insertManyDocuments(documents, database_name, collection_name);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }

    // bson_destroy(document);
}

void DataBase::markDocumentsProcessed(std::vector<bson_t *> &documents, const std::string &database_name, const std::string &collection_name)
{
    std::vector<std::pair<bson_t *, bson_t *>> filterAndUpdateDocuments{};
    bson_error_t error;

    for (auto document : documents)
    {
        bson_t *query = bson_new(); // Create a new bson_t for each query
        bson_t *update = bson_new();
        bson_iter_t iter;
        if (bson_iter_init(&iter, document) && bson_iter_find(&iter, "_id"))
        {
            const bson_oid_t *oid = bson_iter_oid(&iter);
            BSON_APPEND_OID(query, "_id", oid);
            if (update)
            {
                bson_t child;
                BSON_APPEND_DOCUMENT_BEGIN(update, "$set", &child);
                BSON_APPEND_BOOL(&child, "processed", true);
                bson_append_document_end(update, &child);
                filterAndUpdateDocuments.emplace_back(query, update);
            }
            else
            {
                std::cerr << "Failed to create BSON update object." << std::endl;
                bson_destroy(query);
            }
        }
        else
        {
            std::cerr << "Document doesn't contain an _id field" << std::endl;
            bson_destroy(query);
        }
    }

    mongoc_collection_t *collection = mongoc_client_get_collection(m_client, database_name.c_str(), collection_name.c_str());
    insertOrUpdateManyDocuments(filterAndUpdateDocuments, database_name, collection_name);
}

std::string DataBase::extractContentFromIndexDocument(const bson_t *document)
{
    std::vector<std::string> fields{"title", "description", "content", "keywords"};
    std::string content;
    bson_iter_t iter;

    for (const auto &field : fields)
    {
        if (bson_iter_init_find(&iter, document, field.c_str()))
        {
            if (BSON_ITER_HOLDS_UTF8(&iter))
            {
                content += bson_iter_utf8(&iter, nullptr);
                content += ' ';
            }
        }
        else
        {
            std::cerr << "Field: " << field << " not found in document." << std::endl;
        }
    }

    return content;
}

std::string DataBase::extractIndexFromIndexDocument(const bson_t *document)
{
    std::string docId;
    bson_iter_t iter;

    if (bson_iter_init_find(&iter, document, "_id"))
    {
        if (BSON_ITER_HOLDS_UTF8(&iter))
        {
            docId = bson_iter_utf8(&iter, nullptr);
        }
        else if (BSON_ITER_HOLDS_OID(&iter))
        {
            const bson_oid_t *oid;
            oid = bson_iter_oid(&iter);
            char oid_str[25];
            bson_oid_to_string(oid, oid_str);
            docId = oid_str;
        }
        else
        {
            std::cerr << "_id field is not a UTF-8 string or ObjectId." << std::endl;
        }
    }
    else
    {
        std::cerr << "_id field not found in document." << std::endl;
    }

    return docId;
}

Document DataBase::extractDocument(const bson_t *document)
{
    std::vector<std::string> fields{"_id", "title", "description", "content", "url"};
    Document resultDocument{};
    bson_iter_t iter;

    for (const auto &field : fields)
    {
        if (bson_iter_init_find(&iter, document, field.c_str()))
        {
            if (BSON_ITER_HOLDS_UTF8(&iter))
            {
                documentFieldSetters[field](resultDocument, static_cast<std::string>(bson_iter_utf8(&iter, nullptr)));
            }
            else if (BSON_ITER_HOLDS_OID(&iter))
            {
                const bson_oid_t *oid;
                oid = bson_iter_oid(&iter);
                char oid_str[25];
                bson_oid_to_string(oid, oid_str);
                documentFieldSetters[field](resultDocument, static_cast<std::string>(oid_str));
            }
        }

        else
        {
            std::cerr << "Field: " << field << " not found in document." << std::endl;
        }
    }

    return resultDocument;
}

std::string DataBase::getConnectionString()
{
    std::lock_guard<std::mutex> lock(dbMutex);
    return m_connection_string;
}

void DataBase::destroyConnection()
{
    mongoc_cleanup();
}
