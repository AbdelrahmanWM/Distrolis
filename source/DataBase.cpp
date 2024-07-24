#include "DataBase.h"

DataBase *DataBase::db = nullptr;

DataBase::DataBase(const std::string &connectionString, const std::string &database_name, const std::string &collection_name)
    : m_database_name(database_name), m_collection_name(collection_name)
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

DataBase::~DataBase()
{

    if (m_client)
    {
        mongoc_client_destroy(m_client);
    }
    mongoc_cleanup();
}

DataBase *DataBase::getInstance(const std::string &connectionString, const std::string &database_name, const std::string &collection_name)
{
    if (!db)
    {
        db = new DataBase(connectionString, database_name, collection_name);
    }
    return db;
}

void DataBase::destroyInstance()
{
    delete db;
    db = nullptr;
}

void DataBase::insertDocument(const bson_t *document, const std::string &collectionName) const
{
    std::cout << document << "\n";
    mongoc_collection_t *collection = nullptr;
    bson_error_t error;
    bool insertSuccess = false;
    try
    {
        collection = mongoc_client_get_collection(m_client, "SearchEngine", m_collection_name.c_str());
        if (!collection)
        {
            std::cerr << "Failed to get collection: " << m_collection_name << std::endl;
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

std::vector<bson_t> DataBase::getAllDocuments() const
{
    std::vector<bson_t> documents{};
    mongoc_collection_t *collection = nullptr;
    mongoc_cursor_t *cursor = nullptr;
    try
    {
        collection = mongoc_client_get_collection(m_client, m_database_name.c_str(), m_collection_name.c_str());
        if (!collection)
        {
            std::cerr << "Failed to get collection: " << m_collection_name << std::endl;
            return {};
        }
        cursor = mongoc_collection_find_with_opts(collection, bson_new(), nullptr, nullptr);
        if (!cursor)
        {
            std::cerr << "Failed to get cursor to collection: " << m_collection_name << std::endl;
            return {};
        }

        const bson_t *doc;
        while (mongoc_cursor_next(cursor, &doc))
        {
            bson_t *copied_doc = bson_copy(doc);
            documents.push_back(*copied_doc);
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

void DataBase::clearCollection() const
{
    mongoc_collection_t *collection = nullptr;
    try
    {
        collection = mongoc_client_get_collection(m_client, m_database_name.c_str(), m_collection_name.c_str());
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

void DataBase::saveInvertedIndex(const std::unordered_map<std::string, std::vector<Posting>> &index) const
{
    std::cout << "are we here?\n";
    bson_t *document = bson_new();
    if (!document)
    {
        std::cerr << "Failed to create BSON document." << std::endl;
        return;
    }

    try
    {
        for (const auto &[term, postings] : index)
        {
            std::cout << "TERM " << term << '\n';
            bson_t term_doc;
            BSON_APPEND_ARRAY_BEGIN(document, term.c_str(), &term_doc);
            int i = 0;
            for (const auto &posting : postings)
            {

                bson_t posting_doc;
                std::string key = std::to_string(i++);
                BSON_APPEND_DOCUMENT_BEGIN(&term_doc, key.c_str(), &posting_doc);

                BSON_APPEND_UTF8(&posting_doc, "docId", posting.docId.c_str());
                BSON_APPEND_INT32(&posting_doc, "count", posting.count);
                bson_append_document_end(&term_doc, &posting_doc);
            }
            bson_append_array_end(document, &term_doc);
        }
        insertDocument(document, "Index");
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }

    bson_destroy(document);
}

std::string DataBase::extractContentFromIndexDocument(const bson_t &document) const
{
    std::vector<std::string> fields{"title", "description", "content", "keywords"};
    std::string content;
    bson_iter_t iter;

    for (const auto &field : fields)
    {
        if (bson_iter_init_find(&iter, &document, field.c_str()))
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

std::string DataBase::extractIndexFromIndexDocument(const bson_t &document) const
{
    std::string docId;
    bson_iter_t iter;

    if (bson_iter_init_find(&iter, &document, "_id"))
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
