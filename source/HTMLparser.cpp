#include "HTMLParser.h"
#include "URLParser.h"
HTMLParser::HTMLParser()
{
    xmlInitParser();
}

HTMLParser::~HTMLParser()
{
    xmlCleanupParser();
}
bson_t *HTMLParser::getPageDocument(const std::string &htmlContent, const std::string &url) const
{
    return std::move(createBSONFromDocument(extractElements(htmlContent, url)));
}
void HTMLParser::extractAndStorePageDetails(const std::string &htmlContent, const std::string &url, DataBase *&db, const std::string &database_name, const std::string &collection_name) const
{

    try
    {
        documentStructure document = extractElements(htmlContent, url);

        db->insertDocument(std::move(createBSONFromDocument(document)), database_name, collection_name);
    }
    catch (std::runtime_error &ex)
    {
        throw;
    }
}

std::vector<std::string> HTMLParser::extractLinksFromHTML(const std::string &htmlContent) const
{
    std::vector<std::string> links{};
    try
    {
        links = extractLinks(htmlContent);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    return std::move(links);
}
std::vector<std::string> HTMLParser::extractRobotsTxtLinks(const std::string &robotsTxt) const
{
    std::vector<std::string> urls{};
    size_t startIndex = robotsTxt.find("User-agent: *");
    size_t endIndex = robotsTxt.find("User-agent:", startIndex + 1);
    if (endIndex==std::string::npos){
        endIndex=robotsTxt.size()-1;
    }
    std::string url{};
    size_t urlStartIndex{};
    size_t urlEndIndex{};
    while (startIndex < endIndex)
    {
        size_t index= robotsTxt.find("Disallow: ", startIndex);
        if(index!=urlStartIndex)break;
        urlStartIndex=index;
        
        if (urlStartIndex == std::string::npos)break;
        if (urlStartIndex < endIndex)
        {
            // std::cout << urlStartIndex << "," << urlEndIndex << "\n";
            url = "";
            urlStartIndex += 10;
            urlEndIndex = robotsTxt.find("\n", urlStartIndex);

          
            url = robotsTxt.substr(urlStartIndex, urlEndIndex - urlStartIndex);
            //   std::cout << "###"<<url << "###\n";
            if (!URLParser::isURL(url))
            {
                break;
            }
            urls.push_back(url);
            startIndex = urlEndIndex + 1;
        }
        else
            break;
    }
    return std::move(urls);
}

HTMLParser::documentStructure HTMLParser::extractElements(const ::std::string &htmlContent, const std::string &url) const
{
    documentStructure document{};
    xmlXPathContextPtr xpathCtx = nullptr;
    htmlDocPtr doc = nullptr;
    try
    {
        doc = loadHtmlDocument(htmlContent);
        if (doc != nullptr)
        {
            xpathCtx = createXPathContext(doc);
            document.title = extractElement(doc, xpathCtx, "//title");
            document.description = extractElement(doc, xpathCtx, "//meta[@name='description']/@content");
            document.keywords = extractElement(doc, xpathCtx, "//meta[@name='keywords']/@content");
            document.author = extractElement(doc, xpathCtx, "//meta[@name='author']/@content");
            document.publication_date = extractElement(doc, xpathCtx, "//meta[@name='publication_date']/@content");
            document.last_modification_date = extractElement(doc, xpathCtx, "//meta[@name='last_modification_date']/@content");
            document.language = extractElement(doc, xpathCtx, "//html/@lang");
            document.content_type = extractElement(doc, xpathCtx, "//meta[@name='content-type']/@content");
            document.tags = extractElement(doc, xpathCtx, "//meta[@name='tags']/@content");
            document.image_links = extractElement(doc, xpathCtx, "//img/@src");
            document.content = extractText(doc);
            document.url = url;
            document.processed = false;
        }
    }
    catch (std::exception &ex)
    {
        std::cout << "Error: " << ex.what() << '\n';
        throw;
    }
    freeHtmlDocumentContextObject(doc, xpathCtx, nullptr);
    return document;
}
std::string HTMLParser::extractElement(const htmlDocPtr &doc, const xmlXPathContextPtr &xpathCtx, const std::string &xpathExpr) const
{

    std::string element{""};
    xmlXPathObjectPtr xpathObj = nullptr;
    try
    {
        xpathObj = evaluateXPathExpression(xpathCtx, xpathExpr);

        if (!xpathObj)
        {
            return "";
        }

        if (xpathObj && xpathObj->nodesetval && xpathObj->nodesetval->nodeNr > 0)
        {
            element = (char *)xmlNodeGetContent(xpathObj->nodesetval->nodeTab[0]);
        }
        else
        {

            // std::cerr << "No matching element found for XPath: " << xpathExpr << std::endl;
        }
    }
    catch (std::exception &ex)
    {
        std::cout << "Error: " << ex.what() << '\n';
        throw;
    }
    freeHtmlDocumentContextObject(nullptr, nullptr, xpathObj);
    return element;
}
void HTMLParser::extractTextNodes(xmlNodePtr node, std::string &output) const
{
    const std::unordered_set<std::string> HTMLSkepTags = getHTMLSkipTags();
    for (xmlNode *cur = node; cur; cur = cur->next)
    {

        if (cur->type == XML_ELEMENT_NODE && HTMLSkepTags.find((const char *)cur->name) != HTMLSkepTags.end())
        {
            continue;
        }
        if (cur->type == XML_TEXT_NODE)
        {
            output = output + " " + (const char *)cur->content;
        }

        extractTextNodes(cur->children, output);
    }
}

const std::unordered_set<std::string> &HTMLParser::getHTMLSkipTags()
{
    static const std::unordered_set<std::string> HTMLSkipTags = {
        // Navigation and Layout Elements
        "nav", "header", "footer", "aside", "menu",

        // Interactive Elements
        "button", "input", "select", "textarea", "form", "details", "summary",

        // Script and Style Elements
        "script", "style", "link",

        // Media and Embeds
        "img", "video", "audio", "embed", "object", "iframe",

        // Advertisement and Tracking Elements
        "ins", "noscript",

        // Other Miscellaneous Elements
        "hr", "small"};
    return HTMLSkipTags;
}

std::string HTMLParser::extractText(const htmlDocPtr &doc) const
{

    if (!doc)
    {
        std::cerr << "Failed to parse HTML\n";
        return "";
    }
    xmlNodePtr root = xmlDocGetRootElement(doc);
    std::string textContent;
    extractTextNodes(root, textContent);

    return textContent;
}
xmlDocPtr HTMLParser::loadHtmlDocument(const std::string &htmlContent) const
{

    xmlDocPtr doc = htmlReadMemory(htmlContent.c_str(), htmlContent.size(), nullptr, nullptr, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    if (doc == nullptr)
    {
        std::cerr << "Failed to parse HTML document.\n";
    }
    return doc;
}

xmlXPathContextPtr HTMLParser::createXPathContext(const xmlDocPtr &doc) const
{
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == nullptr)
    {
        throw std::runtime_error("Failed to create XPath context.");
    }
    return xpathCtx;
}

xmlXPathObjectPtr HTMLParser::evaluateXPathExpression(const xmlXPathContextPtr &xpathCtx, const std::string &xpathExpr) const
{
    xmlXPathObjectPtr xpathObj = nullptr;
    try
    {
        xpathObj = xmlXPathEvalExpression(reinterpret_cast<const xmlChar *>(xpathExpr.c_str()), xpathCtx);
        if (xpathObj == nullptr)
        {
            throw std::runtime_error("Failed to evaluate xPath expression: " + xpathExpr);
        }
        return xpathObj;
    }
    catch (std::exception &ex)
    {
        std::cout << "Error: " << ex.what() << '\n';
        if (xpathObj != nullptr)
        {
            xmlXPathFreeObject(xpathObj);
        }
        return nullptr;
    }
}

std::vector<std::string> HTMLParser::extractLinks(const std::string &htmlContent) const
{
    std::vector<std::string> links;
    xmlDocPtr doc = loadHtmlDocument(htmlContent);
    if (doc == nullptr)
        return {};
    xmlXPathContextPtr xpathCtx = createXPathContext(doc);
    std::string xpathEpxr = "//a[@href]";
    xmlXPathObjectPtr xpathObj = evaluateXPathExpression(xpathCtx, xpathEpxr);
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    for (int i = 0; i < nodes->nodeNr; ++i)
    {
        xmlNodePtr node = nodes->nodeTab[i];
        xmlChar *href = xmlGetProp(node, reinterpret_cast<const xmlChar *>("href"));
        if (href != nullptr)
        {
            links.push_back(reinterpret_cast<const char *>(href));
            xmlFree(href);
        }
    }
    freeHtmlDocumentContextObject(doc, xpathCtx, xpathObj);

    return links;
}

void HTMLParser::freeHtmlDocumentContextObject(const htmlDocPtr &doc, const xmlXPathContextPtr &xpathCtx, const xmlXPathObjectPtr &xpathObject) const
{
    if (xpathObject)
        xmlXPathFreeObject(xpathObject);
    if (xpathCtx)
        xmlXPathFreeContext(xpathCtx);
    if (doc)
        xmlFreeDoc(doc);
}

bson_t *HTMLParser::createBSONFromDocument(const documentStructure &doc) const
{
    bson_t *bson = bson_new();

    try
    {
        bson_init(bson);

        bson_append_utf8(bson, "url", -1, doc.url.c_str(), -1);
        bson_append_utf8(bson, "title", -1, doc.title.c_str(), -1);
        bson_append_utf8(bson, "description", -1, doc.description.c_str(), -1);
        bson_append_utf8(bson, "keywords", -1, doc.keywords.c_str(), -1);
        bson_append_utf8(bson, "author", -1, doc.author.c_str(), -1);
        bson_append_utf8(bson, "publication_date", -1, doc.publication_date.c_str(), -1);
        bson_append_utf8(bson, "last_modified_date", -1, doc.last_modification_date.c_str(), -1);
        bson_append_utf8(bson, "content_type", -1, doc.content_type.c_str(), -1);
        bson_append_utf8(bson, "tags", -1, doc.tags.c_str(), -1);
        bson_append_utf8(bson, "image_links", -1, doc.image_links.c_str(), -1);
        bson_append_utf8(bson, "content", -1, doc.content.c_str(), -1);
        bson_append_bool(bson, "processed", -1, false);
        // char* str = bson_as_json(bson, nullptr);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    return bson;
}
