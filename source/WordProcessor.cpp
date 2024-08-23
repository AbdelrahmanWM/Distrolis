#include "WordProcessor.h"

std::vector<std::string> WordProcessor::tokenize(const std::string &content)
{
    std::regex rgx{R"(\b[\w'-]+\b)"};
    auto words_begin = std::sregex_iterator (content.begin(),content.end(),rgx);
    auto words_end = std::sregex_iterator();
    std::string token;
    std::vector<std::string> tokens;
    for(std::sregex_iterator i= words_begin; i != words_end; ++i)
    {
        token = (*i).str();
        if(WordProcessor::isValidWord(token))// Temporary filtration step until better solution is found
        tokens.push_back(token);
    }
    return tokens;

}

bool WordProcessor::isValidWord(std::string word)
{
    return word.size()>3&&word.size()<=20;
}

std::string WordProcessor::normalize(std::string &token)
{
    std::string normalized;
    std::transform(token.begin(), token.end(), std::back_inserter(normalized), ::tolower);
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(), ::ispunct), normalized.end());
    return normalized;
}

std::string WordProcessor::stem(std::string &word)
{
    sb_stemmer *stemmer = sb_stemmer_new("english", nullptr);
    if (!stemmer)
    {
        throw std::runtime_error("Failed to create stemmer.");
    }
    const sb_symbol *input = reinterpret_cast<const sb_symbol *>(word.c_str());
    const sb_symbol *stemmed = sb_stemmer_stem(stemmer, input, word.length());

    if (!stemmed)
    {
        sb_stemmer_delete(stemmer);
        throw std::runtime_error("Failed to stem word.");
    }
    std::string stemmed_word{reinterpret_cast<const char *>(stemmed)};
    sb_stemmer_delete(stemmer);
    return stemmed_word;
}

bool WordProcessor::isStopWord(std::string &word)
{
    return WordProcessor::getStopWords().find(word) !=WordProcessor::getStopWords().end();

}

const std::unordered_set<std::string> &WordProcessor::getStopWords()
{
    static const std::unordered_set<std::string> stopWords = {
        "a", "about", "above", "after", "again", "against", "all", "am", "an", "and", "any", "are", "aren't", "as", "at", 
        "be", "because", "been", "before", "being", "below", "between", "both", "but", "by", 
        "can't", "cannot", "could", "couldn't", "did", "didn't", "do", "does", "doesn't", "doing", "don't", "down", "during", 
        "each", "few", "for", "from", "further", 
        "had", "hadn't", "has", "hasn't", "have", "haven't", "having", "he", "he'd", "he'll", "he's", "her", "here", "here's", "hers", "herself", "him", "himself", "his", 
        "how", "how's", 
        "i", "i'd", "i'll", "i'm", "i've", "if", "in", "into", "is", "isn't", "it", "it's", "its", "itself", 
        "let's", 
        "me", "more", "most", "mustn't", "my", "myself", 
        "no", "nor", "not", 
        "of", "off", "on", "once", "only", "or", "other", "ought", "our", "ours", "ourselves", "out", "over", "own", 
        "same", "shan't", "she", "she'd", "she'll", "she's", "should", "shouldn't", "so", "some", "such", 
        "than", "that", "that's", "the", "their", "theirs", "them", "themselves", "then", "there", "there's", "these", "they", "they'd", "they'll", "they're", "they've", "this", "those", "through", "to", "too", 
        "under", "until", "up", 
        "very", 
        "was", "wasn't", "we", "we'd", "we'll", "we're", "we've", "were", "weren't", "what", "what's", "when", "when's", "where", "where's", "which", "while", "who", "who's", "whom", "why", "why's", "with", "won't", "would", "wouldn't", 
        "you", "you'd", "you'll", "you're", "you've", "your", "yours", "yourself", "yourselves",
        
        "above", "below", "between", "cannot", "couldn't", "didn't", "doesn't", "doing", "during", "further", "hasn't", 
        "haven't", "isn't", "mightn't", "mustn't", "needn't", "shan't", "shouldn't", "wasn't", "weren't", "won't", 
        "wouldn't", "able", "ain", "aren", "can", "couldn", "didn", "doesn", "don", "hadn", "hasn", "haven", 
        "isn", "ma", "mightn", "mustn", "needn", "shan", "shouldn", "wasn", "weren", "won", "wouldn", 
        "above", "across", "after", "along", "among", "around", "as", "at", "before", "behind", "below", 
        "beneath", "beside", "between", "beyond", "by", "despite", "down", "during", "except", "for", 
        "from", "in", "inside", "into", "like", "near", "of", "off", "on", "onto", "outside", "over", 
        "past", "since", "through", "to", "toward", "under", "until", "up", "upon", "with", "within", "without", 
        "the", "then", "there", "where", "when", "which", "while", "who", "whose", "whom", "why", "will", 
        "am", "is", "are", "was", "were", "be", "being", "been", "have", "has", "had", "do", "does", 
        "did", "doing", "would", "could", "should", "can", "may", "might", "must", "shall", "will", "won't",
        "whenever", "wherever", "however", "whoever", "every", "everything", "everyone", "everybody", 
        "all", "almost", "alone", "along", "already", "also", "although", "always", "among", "another", 
        "any", "anybody", "anyone", "anything", "anywhere", "apart", "appear", "appreciate", "appropriate", 
        "are", "aren", "aren't", "around", "as", "aside", "ask", "asking", "associated", "at", "available", 
        "away", "awfully", "back", "be", "became", "because", "become", "becomes", "becoming", "been", 
        "before", "beforehand", "behind", "being", "believe", "below", "beside", "besides", "best", 
        "better", "between", "beyond", "both", "brief", "but", "by", "came", "can", "cannot", "cant", "can’t", 
        "certain", "certainly", "clearly", "com", "come", "comes", "concerning", "consequently", 
        "consider", "considering", "contain", "containing", "contains", "corresponding", "could", "couldn’t", 
        "course", "currently", "definitely", "described", "despite", "did", "didn’t", "different", "do", "does", 
        "doesn’t", "doing", "don’t", "done", "down", "downwards", "during", "each", "edu", "eg", "eight", 
        "either", "else", "elsewhere", "enough", "entirely", "especially", "et", "etc", "even", "ever", 
        "every", "everybody", "everyone", "everything", "everywhere", "ex", "exactly", "example", 
        "except", "far", "few", "fifth", "first", "five", "followed", "following", "follows", "for", 
        "former", "formerly", "forth", "four", "from", "further", "furthermore", "get", "gets", "getting", 
        "given", "gives", "go", "goes", "going", "gone", "got", "gotten", "greetings", "had", "hadn’t", 
        "happens", "hardly", "has", "hasn’t", "have", "haven’t", "having", "he", "he’s", "hello", 
        "help", "hence", "her", "here", "here’s", "hereafter", "hereby", "herein", "hereupon", "hers", 
        "herself", "hi", "him", "himself", "his", "hither", "hopefully", "how", "howbeit", "however", 
        "i", "i’d", "i’ll", "i’m", "i’ve", "ie", "if", "ignored", "immediate", "in", "inasmuch", 
        "inc", "indeed", "indicate", "indicated", "indicates", "inner", "insofar", "instead", "into", 
        "inward", "is", "isn’t", "it", "it’d", "it’ll", "it’s", "its", "itself", "just", "keep", "keeps", 
        "kept", "know", "knows", "known", "last", "lately", "later", "latter", "latterly", "least", 
        "less", "lest", "let", "let’s", "like", "liked", "likely", "little", "look", "looking", "looks", 
        "ltd", "mainly", "many", "may", "maybe", "me", "mean", "meanwhile", "merely", "might", 
        "more", "moreover", "most", "mostly", "much", "must", "my", "myself", "name", "namely", 
        "nd", "near", "nearly", "necessary", "need", "needs", "neither", "never", "nevertheless", 
        "new", "next", "nine", "no", "nobody", "non", "none", "noone", "nor", "normally", "not", 
        "nothing", "novel", "now", "nowhere", "obviously", "of", "off", "often", "oh", "ok", "okay", 
        "old", "on", "once", "one", "ones", "only", "onto", "or", "other", "others", "otherwise", 
        "ought", "our", "ours", "ourselves", "out", "outside", "over", "overall", "own", "particular", 
        "particularly", "per", "perhaps", "placed", "please", "plus", "possible", "presumably", 
        "probably", "provides", "que", "quite", "qv", "rather", "rd", "re", "really", "reasonably", 
        "regarding", "regardless", "regards", "relatively", "respectively", "right", "said", "same", 
        "saw", "say", "saying", "says", "second", "secondly", "see", "seeing", "seem", "seemed", 
        "seeming", "seems", "seen", "self", "selves", "sensible", "sent", "serious", "seriously", 
        "seven", "several", "shall", "she", "should", "shouldn’t", "since", "six", "so", "some", 
        "somebody", "somehow", "someone", "something", "sometime", "sometimes", "somewhat", "somewhere", 
        "soon", "sorry", "specified", "specify", "specifying", "still", "sub", "such", "sup", "sure", 
        "take", "taken", "taking", "tell", "tends", "th", "than", "thank", "thanks", "thanx", "that", 
        "that’s", "thats", "the", "their", "theirs", "them", "themselves", "then", "thence", "there", 
        "there’s", "thereafter", "thereby", "therefore", "therein", "theres", "thereupon", "these", 
        "they", "they’d", "they’ll", "they’re", "they’ve", "think", "third", "this", "thorough", 
        "thoroughly", "those", "though", "three", "through", "throughout", "thru", "thus", "to", 
        "together", "too", "took", "toward", "towards", "tried", "tries", "truly", "try", "trying", 
        "twice", "two", "un", "under", "unfortunately", "unless", "unlikely", "until", "unto", "up", 
        "upon", "us", "use", "used", "useful", "uses", "using", "usually", "value", "various", "very", 
        "via", "viz", "vs", "want", "wants", "was", "wasn’t", "way", "we", "we’d", "we’ll", "we’re", 
        "we’ve", "welcome", "well", "went", "were", "weren’t", "what", "what’s", "whatever", "when", 
        "whence", "whenever", "where", "where’s", "whereafter", "whereas", "whereby", "wherein", 
        "whereupon", "wherever", "whether", "which", "while", "whither", "who", "who’s", "whoever", 
        "whole", "whom", "whose", "why", "will", "willing", "wish", "with", "within", "without", 
        "won’t", "wonder", "would", "wouldn’t", "yes", "yet", "you", "you’d", "you’ll", "you’re", 
        "you’ve", "your", "yours", "yourself", "yourselves", "zero"
    };
    return stopWords;
}
