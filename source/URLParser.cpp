#include "URLParser.h"
#include <iostream>
URLParser::URLParser() {
    
}

void URLParser::checkBaseUrl(const std::string&base_url )
{
if (base_url.empty()) {
        throw std::invalid_argument("Base URL cannot be empty");
    }
}

bool URLParser::isAbsoluteURL(const std::string &url)
{
    const std::regex urlPattern(R"(^(https?://)([a-zA-Z0-9-]+\.)+[a-zA-Z0-9-]+)");

    return std::regex_search(url, urlPattern) && isValidTLD(url);
}

bool URLParser::isProtocolRelativeURL(const std::string& url) {
    return url.find("//") == 0;
}

bool URLParser::isRootRelativeURL(const std::string& url) {
    return !url.empty() && url[0] == '/';
}

bool URLParser::shouldIgnoreURL(const std::string& url) {
    return url.empty() || url == "/" || url[0] == '#';
}


std::string URLParser::normalizeURL(const std::string &url)
{
    std::string normalizeURL {url};
    char lastChar = normalizeURL.back();
    while(lastChar=='*'||lastChar=='$'||lastChar=='/'||lastChar=='?'){
        
        normalizeURL.pop_back();
        lastChar = normalizeURL.back();
    }
    size_t index = normalizeURL.find("/*");
    if(index!=std::string::npos){
        normalizeURL = normalizeURL.substr(0,index);
    }
    return normalizeURL;
}

std::string URLParser::resolveProtocolRelativeURL(const std::string& url,const std::string& base_url) {
    std::regex regex{ "^(http:|https:)" };
    std::smatch match;
    if (std::regex_search(base_url, match, regex)) {
        return match.str(1) + url;
    }
    else {
        throw std::runtime_error("Base URL missing scheme for protocol-relative URL");
    }
}

std::string URLParser::resolveRootRelativeURL(const std::string& url,const std::string& base_url) {
    std::regex domainRegex{ R"(^([a-zA-Z]+://[^/]+))" };
    std::smatch match;
    if (std::regex_search(base_url, match, domainRegex)) {
        return match.str(1) + url;
    }
    else {
        throw std::runtime_error("Base URL invalid for root-relative URL");
    }
}

std::string URLParser::resolvePathRelativeURL(const std::string& url,const std::string& base_url) {
    std::string base = base_url;
    std::size_t last_slash = base.find_last_of('/');
    if (last_slash != std::string::npos) {
        base = base.substr(0, last_slash + 1);
    }
    return base + url;
}

std::string URLParser::convertToAbsoluteURL(const std::string& url,const std::string& base_url) {
    std::string result{};
    if (shouldIgnoreURL(url)) {
        result = base_url;
    }

    else if (isAbsoluteURL(url)) {
        result = url;
    }
    else if (isProtocolRelativeURL(url)) {
        result = resolveProtocolRelativeURL(url,base_url);
    }
    else if (isRootRelativeURL(url)) {
        result = resolveRootRelativeURL(url,base_url);
    }
    else {
        result = resolvePathRelativeURL(url,base_url);
    }
    // std::cout<<url<<"+"<<base_url<<"="<<result<<'\n';
    return normalizeURL(result);

}


bool URLParser::isDomainURL(const std::string& base_url)
{
    std::regex domainRegex {R"(^(https?:\/\/)?([a-zA-Z0-9-]+\.)+[a-zA-Z]{2,}\/?$)"};
    std::smatch match;
    return std::regex_match(base_url,domainRegex)&& isValidTLD(base_url);
}

std::string URLParser::getRobotsTxtURL(const std::string& base_url)
{

    if(base_url[base_url.length()-1]=='/')return base_url+"robots.txt";
    else return base_url+"/robots.txt";
}
bool URLParser::isValidTLD(const std::string &domain)
{
    size_t dotPos = domain.rfind('.');
    if(dotPos == std::string::npos){
        return false;
    }
    std::string tld = domain.substr(dotPos+1);
       size_t slashPos = tld.find('/');
    if (slashPos != std::string::npos) {
        tld = tld.substr(0, slashPos);
    }
    std::transform(tld.begin(),tld.end(),tld.begin(),::toupper);
    return getTLDsList().find(tld)!=getTLDsList().end();
}

std::unordered_set<std::string> URLParser::getTLDsList()
{
    static std::unordered_set<std::string> domainSet = {
        "AAA", "AARP", "ABB", "ABBOTT", "ABBVIE", "ABC", "ABLE", "ABOGADO",
        "ABUDHABI", "AC", "ACADEMY", "ACCENTURE", "ACCOUNTANT", "ACCOUNTANTS",
        "ACO", "ACTOR", "AD", "ADS", "ADULT", "AE", "AEG", "AERO", "AETNA",
        "AF", "AFL", "AFRICA", "AG", "AGAKHAN", "AGENCY", "AI", "AIG", "AIRBUS",
        "AIRFORCE", "AIRTEL", "AKDN", "AL", "ALIBABA", "ALIPAY", "ALLFINANZ",
        "ALLSTATE", "ALLY", "ALSACE", "ALSTOM", "AM", "AMAZON", "AMERICANEXPRESS",
        "AMERICANFAMILY", "AMEX", "AMFAM", "AMICA", "AMSTERDAM", "ANALYTICS",
        "ANDROID", "ANQUAN", "ANZ", "AO", "AOL", "APARTMENTS", "APP", "APPLE",
        "AQ", "AQUARELLE", "AR", "ARAB", "ARAMCO", "ARCHI", "ARMY", "ARPA",
        "ART", "ARTE", "AS", "ASDA", "ASIA", "ASSOCIATES", "AT", "ATHLETA",
        "ATTORNEY", "AU", "AUCTION", "AUDI", "AUDIBLE", "AUDIO", "AUSPOST",
        "AUTHOR", "AUTO", "AUTOS", "AW", "AWS", "AX", "AXA", "AZ", "AZURE",
        "BA", "BABY", "BAIDU", "BANAMEX", "BAND", "BANK", "BAR", "BARCELONA",
        "BARCLAYCARD", "BARCLAYS", "BAREFOOT", "BARGAINS", "BASEBALL", "BASKETBALL",
        "BAUHAUS", "BAYERN", "BB", "BBC", "BBT", "BBVA", "BCG", "BCN", "BD",
        "BE", "BEATS", "BEAUTY", "BEER", "BENTLEY", "BERLIN", "BEST", "BESTBUY",
        "BET", "BF", "BG", "BH", "BHARTI", "BI", "BIBLE", "BID", "BIKE",
        "BING", "BINGO", "BIO", "BIZ", "BJ", "BLACK", "BLACKFRIDAY", "BLOCKBUSTER",
        "BLOG", "BLOOMBERG", "BLUE", "BM", "BMS", "BMW", "BN", "BNPPARIBAS",
        "BO", "BOATS", "BOEHRINGER", "BOFA", "BOM", "BOND", "BOO", "BOOK",
        "BOOKING", "BOSCH", "BOSTIK", "BOSTON", "BOT", "BOUTIQUE", "BOX", "BR",
        "BRADESCO", "BRIDGESTONE", "BROADWAY", "BROKER", "BROTHER", "BRUSSELS",
        "BS", "BT", "BUILD", "BUILDERS", "BUSINESS", "BUY", "BUZZ", "BV",
        "BW", "BY", "BZ", "BZH", "CA", "CAB", "CAFE", "CAL", "CALL", "CALVINKLEIN",
        "CAM", "CAMERA", "CAMP", "CANON", "CAPETOWN", "CAPITAL", "CAPITALONE",
        "CAR", "CARAVAN", "CARDS", "CARE", "CAREER", "CAREERS", "CARS", "CASA",
        "CASE", "CASH", "CASINO", "CAT", "CATERING", "CATHOLIC", "CBA", "CBN",
        "CBRE", "CC", "CD", "CENTER", "CEO", "CERN", "CF", "CFA", "CFD", "CG",
        "CH", "CHANEL", "CHANNEL", "CHARITY", "CHASE", "CHAT", "CHEAP", "CHINTAI",
        "CHRISTMAS", "CHROME", "CHURCH", "CI", "CIPRIANI", "CIRCLE", "CISCO",
        "CITADEL", "CITI", "CITIC", "CITY", "CK", "CL", "CLAIMS", "CLEANING",
        "CLICK", "CLINIC", "CLINIQUE", "CLOTHING", "CLOUD", "CLUB", "CLUBMED",
        "CM", "CN", "CO", "COACH", "CODES", "COFFEE", "COLLEGE", "COLOGNE",
        "COM", "COMMBANK", "COMMUNITY", "COMPANY", "COMPARE", "COMPUTER", "COMSEC",
        "CONDOS", "CONSTRUCTION", "CONSULTING", "CONTACT", "CONTRACTORS", "COOKING",
        "COOL", "COOP", "CORSICA", "COUNTRY", "COUPON", "COUPONS", "COURSES", "CPA",
        "CR", "CREDIT", "CREDITCARD", "CREDITUNION", "CRICKET", "CROWN", "CRS",
        "CRUISE", "CRUISES", "CU", "CUISINELLA", "CV", "CW", "CX", "CY", "CYMRU",
        "CYOU", "CZ", "DABUR", "DAD", "DANCE", "DATA", "DATE", "DATING", "DATSUN",
        "DAY", "DCLK", "DDS", "DE", "DEAL", "DEALER", "DEALS", "DEGREE",
        "DELIVERY", "DELL", "DELOITTE", "DELTA", "DEMOCRAT", "DENTAL", "DENTIST",
        "DESI", "DESIGN", "DEV", "DHL", "DIAMONDS", "DIET", "DIGITAL", "DIRECT",
        "DIRECTORY", "DISCOUNT", "DISCOVER", "DISH", "DIY", "DJ", "DK", "DM",
        "DNP", "DO", "DOCS", "DOCTOR", "DOG", "DOMAINS", "DOT", "DOWNLOAD",
        "DRIVE", "DTV", "DUBAI", "DUNLOP", "DUPONT", "DURBAN", "DVAG", "DVR", "DZ",
        "EARTH", "EAT", "EC", "ECO", "EDEKA", "EDU", "EDUCATION", "EE", "EG",
        "EMAIL", "EMERCK", "ENERGY", "ENGINEER", "ENGINEERING", "ENTERPRISES", "EPSON",
        "EQUIPMENT", "ER", "ERICSSON", "ERNI", "ES", "ESQ", "ESTATE", "ET", "EU",
        "EUROVISION", "EUS", "EVENTS", "EXCHANGE", "EXPERT", "EXPOSED", "EXPRESS",
        "EXTRASPACE", "FAGE", "FAIL", "FAIRWINDS", "FAITH", "FAMILY", "FAN", "FANS",
        "FARM", "FARMERS", "FASHION", "FAST", "FEDEX", "FEEDBACK", "FERRARI",
        "FERRERO", "FI", "FIDELITY", "FIDO", "FILM", "FINAL", "FINANCE", "FINANCIAL",
        "FIRE", "FIRESTONE", "FIRMDALE", "FISH", "FISHING", "FIT", "FITNESS", "FJ",
        "FK", "FLICKR", "FLIGHTS", "FLIR", "FLORIST", "FLOWERS", "FLY", "FM", "FO",
        "FOO", "FOOD", "FOOTBALL", "FORD", "FOREX", "FORSALE", "FORUM", "FOUNDATION",
        "FOX", "FR", "FREE", "FRESENIUS", "FRL", "FROGANS", "FRONTIER", "FTR",
        "FUJITSU", "FUN", "FUND", "FURNITURE", "FUTBOL", "FYI", "GA", "GAL", "GALLERY",
        "GALLO", "GALLUP", "GAME", "GAMES", "GAP", "GARDEN", "GAY", "GB", "GBIZ",
        "GD", "GDN", "GE", "GEA", "GENT", "GENTING", "GEORGE", "GGP", "GH", "GI",
        "GIFT", "GIFTED", "GIGA", "GIGS", "GILLETTE", "GINKGO", "GIO", "GIT",
        "GIVE", "GIVING", "GL", "GLASS", "GLASSFIBER", "GLASSWORKS", "GLAZER",
        "GLOBE", "GLOBE.COM", "GMAIL", "GMBH", "GMX", "GNC", "GOLF", "GOOGLE",
        "GOV", "GOVERNMENT", "GP", "GQ", "GR", "GRADUATE", "GRAPHICDESIGN", "GREAT",
        "GREEN", "GRILL", "GROUP", "GU", "GUARD", "GUIDE", "GUN", "GURU", "HA",
        "HABITAT", "HACK", "HACKER", "HAIR", "HAMBURG", "HAMPTON", "HANGOUT",
        "HARDWARE", "HAWAII", "HEALTH", "HEALTHCARE", "HELP", "HERMES", "HIGHSPEED",
        "HILTON", "HINT", "HIS", "HIT", "HIV", "HK", "HM", "HNL", "HOBBY", "HOMES",
        "HONDA", "HONEYWELL", "HOOD", "HOP", "HOSPITAL", "HOST", "HOTEL", "HOUSE",
        "HOW", "HSBC", "HTC", "HU", "HUB", "HUGO", "HUMAN", "HUMANITY", "HUNT",
        "HYATT", "HYUNDAI", "IAC", "ICBC", "ICE", "ICON", "ID", "IE", "IEEE",
        "IFM", "IG", "IM", "IMAX", "IMF", "IN", "INBOX", "INDUSTRY", "INFO",
        "ING", "INK", "INSTITUTE", "INSURANCE", "INT", "INTERNATIONAL", "INVESTMENTS",
        "IO", "IP", "IR", "IRISH", "IS", "ISLAND", "ISP", "IT", "JACOBS", "JAGUAR",
        "JANUARY", "JAZZ", "JE", "JEWELRY", "JOBS", "JP", "JPL", "JSE", "JSTOR",
        "JUKI", "JUMP", "JUNO", "JUPITER", "K12", "KAC", "KARMA", "KASE", "KAZ",
        "KDS", "KE", "KEN", "KFC", "KG", "KIA", "KID", "KINDER", "KINDLE", "KITCHEN",
        "KIWI", "KLA", "KM", "KMT", "KN", "KO", "KONICA", "KONICA-MINOLTA", "KR",
        "KREMLIN", "KROGER", "KW", "KY", "KZ", "LAC", "LAND", "LASALLE", "LATAM",
        "LATINO", "LAW", "LAWYER", "LB", "LC", "LD", "LE", "LEAGUE", "LEBANON",
        "LEGAL", "LEHMAN", "LELO", "LEND", "LEX", "LGBT", "LI", "LIFE", "LIFESTYLE",
        "LIGHT", "LIVE", "LIVING", "LLC", "LM", "LOANS", "LOC", "LOOP", "LOVE",
        "LOWES", "LR", "LS", "LT", "LU", "LUXURY", "LY", "MA", "MALL", "MAN",
        "MANAGEMENT", "MANGO", "MARKET", "MARKETING", "MARRIOTT", "MARS", "MARVEL",
        "MAT", "MBA", "MD", "ME", "MED", "MEDIA", "MEDICAL", "MEET", "MEETUP",
        "MEGA", "MEL", "MELBOURNE", "MERCEDES", "MERRILL", "MESA", "MET",
        "MEX", "MI", "MICROSOFT", "MIDDLE", "MIL", "MINI", "MINT", "MIRAGE", "MOBILE",
        "MOD", "MOI", "MOM", "MON", "MONITOR", "MONSANTO", "MONTBLANC", "MOOD",
        "MOON", "MORGAN", "MORTGAGE", "MOSCOW", "MOTOR", "MOV", "MOVIE", "MP",
        "MQ", "MR", "MS", "MT", "MU", "MUSEUM", "MV", "MW", "MX", "MY", "NAME",
        "NASA", "NASCAR", "NAT", "NATIONAL", "NET", "NETBANK", "NETWORK", "NEU",
        "NEW", "NEWS", "NG", "NI", "NICE", "NIKON", "NINJA", "NL", "NO", "NOKIA",
        "NOM", "NORD", "NORTON", "NOW", "NP", "NR", "NT", "NU", "NY", "NZ", "OB",
        "OCEAN", "OFFICE", "OM", "ONE", "ONLINE", "OO", "OR", "ORG", "ORGANIC",
        "OS", "OT", "OTE", "OTG", "OV", "OVER", "PADI", "PA", "PAG", "PAR",
        "PARTNER", "PARTS", "PAS", "PE", "PEACE", "PEACH", "PEARL", "PEMBROKE",
        "PEOPLE", "PERFUME", "PERK", "PERSONAL", "PET", "PH", "PHOTOS", "PHYSICAL",
        "PI", "PILOT", "PING", "PIZZA", "PL", "PLANT", "PLATFORM", "PLAY",
        "PLAYSTATION", "PLUMBING", "PM", "PN", "POLITICS", "POLY", "POST", "PR",
        "PRACTICE", "PRESS", "PRO", "PRODUCTS", "PROF", "PROJECTS", "PROMO",
        "PROPERTY", "PROTECT", "PTO", "PUB", "PUBLISHING", "PW", "QA", "QATAR",
        "QUEST", "RACING", "RADIO", "RE", "REALTOR", "RECORD", "REHAB", "REIT",
        "RELO", "RENT", "RESTAURANT", "RETAIL", "REVIEW", "RICE", "RING",
        "RIO", "RISK", "RO", "ROBOT", "ROLEX", "ROLLS", "ROM", "RUGBY", "RU",
        "RUSSIA", "RW", "RY", "SA", "SAB", "SAFETY", "SALES", "SALON", "SAM",
        "SAMSUNG", "SAN", "SAND", "SANTA", "SARL", "SAS", "SAVE", "SAY",
        "SCHOOL", "SCIENCE", "SCOR", "SEARCH", "SEAT", "SECURITY", "SEE", "SELL",
        "SEMPRE", "SERVICE", "SES", "SEXY", "SG", "SH", "SHOP", "SHOES",
        "SHOW", "SHR", "SI", "SIC", "SILVER", "SIM", "SK", "SKATE", "SKIN",
        "SL", "SLC", "SMART", "SMILE", "SN", "SO", "SOCIAL", "SOFTWARE", "SOLAR",
        "SOLUTIONS", "SONY", "SOS", "SPACE", "SPAIN", "SPEECH", "SQUARE", "SR",
        "SS", "ST", "STADIUM", "STANFORD", "STATE", "STC", "STUDIO", "STYLE",
        "SU", "SUNG", "SUPPORT", "SURF", "SY", "SYMPHONY", "SYSTEMS", "SZ",
        "TAX", "TECH", "TEL", "TELECOM", "TELEPHONE", "TEXAS", "THE", "THEATRE",
        "THINK", "TI", "TICKETS", "TIDAL", "TIME", "TIPS", "TO", "TODAY",
        "TOOLS", "TOP", "TOY", "TR", "TRADE", "TRAVEL", "TREND", "TRUST", "TT",
        "TV", "TUNE", "TUR", "TUSHU", "TW", "TZ", "UA", "UBS", "UG", "UK",
        "UM", "UN", "UNIVERSITY", "UNO", "US", "UY", "UZ", "VA", "VACATIONS",
        "VAL", "VE", "VEN", "VENTURES", "VERISIGN", "VERY", "VF", "VG", "VI",
        "VIC", "VILLA", "VIP", "VISA", "VN", "VODAFONE", "VOLE", "VOTE",
        "VOV", "VY", "WAG", "WAL", "WALL", "WALTER", "WASTE", "WATCH",
        "WATER", "WE", "WEB", "WELLS", "WF", "WHOLESALE", "WIFI", "WIN", "WORK",
        "WORLD", "WORLDS", "WWE", "WWW", "XBOX", "XE", "XEROX", "XIN", "XOXO",
        "YACHT", "YAHOO", "YE", "YOGA", "YT", "YU", "ZA", "ZARA", "ZERO", "ZIP"
    };
    return domainSet;
}

