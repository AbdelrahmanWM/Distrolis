#include "DataBase.h"
#include "ThreadPool.h";

DataBase *DataBase::db = nullptr;

DataBase::DataBase(const std::string &connectionString)
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

void DataBase::processWordDocuments(std::vector<bson_t *> &documents, std::mutex &documentsMutex, const std::string &term, const std::unordered_map<std::string, std::vector<int>> &map)
{
    try
    {
        std::cout << std::this_thread::get_id() << "\n";
        if (term == "")
            return;
        bson_t *term_doc = bson_new();
        if (!term_doc)
        {
            std::cerr << "Failed to create BSON document." << std::endl;
        }
        bson_t postings_array;
        BSON_APPEND_ARRAY_BEGIN(term_doc, term.c_str(), &postings_array);
        // BSON_APPEND_ARRAY_BEGIN(document, term.c_str(), &term_doc);
        int i = 0;
        for (const auto &[docId, positions] : map)
        {
            if (docId == "" || positions.size() == 0)
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
        {
            std::lock_guard<std::mutex> lock(documentsMutex);
            if (term_doc != nullptr)
            {
                documents.push_back(term_doc);
            }
            else
            {
                std::cerr << "Didn't insert the faulty document\n";
            }
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
    mongoc_cleanup();
}

DataBase *DataBase::getInstance(const std::string &connectionString)
{
    if (!db)
    {
        db = new DataBase(connectionString);
    }
    return db;
}

void DataBase::destroyInstance()
{
    delete db;
    db = nullptr;
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
void DataBase::insertManyDocuments(std::vector<bson_t *> documents, const std::string &database_name, const std::string &collection_name)
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
        std::cout << "HERE\n";
        // for (const bson_t *doc : documentPointers)
        // {
        //     if (doc == nullptr)
        //     {
        //         std::cout << "Found a null document pointer in documentPointers\n";
        //         // Handle error
        //     }
        // }
        insertSuccess = mongoc_collection_insert_many(collection, documentPointers.data(), documentPointers.size(), nullptr, nullptr, &error);
        std::cout << "HERE\n";
        if (!insertSuccess)
        {
            std::cerr << "Failed to insert documents: " << error.message << std::endl;
        }
        else
        {
            std::cout << "Documents inserted successfully." << std::endl;
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

std::vector<bson_t *> DataBase::getDocumentsByIds(const std::string &database_name, const std::string &collection_name, const std::vector<std::string> &ids)
{
    bson_t *filter = DataBase::createIdFilter(ids);
    return DataBase::getAllDocuments(database_name, collection_name, filter);
}

bson_t *DataBase::createIdFilter(const std::vector<std::string> &ids)
{
    bson_t *filter = bson_new();
    bson_t *or_array = bson_new();
    for (const auto &id : ids)
    {
        bson_t *id_doc = BCON_NEW("_id", BCON_UTF8(id.c_str()));
        bson_append_document(or_array, "", -1, id_doc);
        bson_destroy(id_doc);
    }
    BSON_APPEND_ARRAY(filter, "$or", or_array);
    bson_destroy(or_array);
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
    std::mutex documentsMutex;
    try
    {
        {
            ThreadPool thread_pool{5};
            for (const auto &[term, map] : index)
            {
                thread_pool.enqueue([this, &documents, &documentsMutex, term, map]
                                    { this->processWordDocuments(documents, documentsMutex, term, map); });
            }
        }
            // insertDocument(document, database_name,collection_name);
            insertManyDocuments(documents, database_name, collection_name);
            for (bson_t *document : documents)
            {
                bson_destroy(document);
            }
        
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }

    // bson_destroy(document);
}

void DataBase::markDocumentProcessed(const bson_t *document, const std::string &database_name, const std::string &collection_name)
{
    std::lock_guard<std::mutex> lock(dbMutex);

    bson_error_t error;
    bson_t *update = NULL;
    bson_t query;
    bson_iter_t iter;

    if (bson_iter_init(&iter, document) && bson_iter_find(&iter, "_id"))
    {
        const bson_oid_t *oid = bson_iter_oid(&iter);
        bson_init(&query);
        BSON_APPEND_OID(&query, "_id", oid);
        update = bson_new();
        bson_t child;
        BSON_APPEND_DOCUMENT_BEGIN(update, "$set", &child);
        BSON_APPEND_BOOL(&child, "processed", true);
        bson_append_document_end(update, &child);

        mongoc_collection_t *collection = mongoc_client_get_collection(m_client, database_name.c_str(), collection_name.c_str());
        if (!mongoc_collection_update_one(collection, &query, update, NULL, NULL, &error))
        {
            std::cerr << "Failed to update document: " << error.message << std::endl;
        }
        else
        {
            std::cout << std::this_thread::get_id() << std::endl;
        }
    }
    else
    {
        std::cerr << "Document doesn't contain an _id field" << std::endl;
        return;
    }
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
