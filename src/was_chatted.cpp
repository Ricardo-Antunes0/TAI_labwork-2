#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <memory>
#include <sstream>

size_t k = 3, alpha = 1;

class ModelData {
    private:
        std::unordered_map<std::string,std::vector<size_t>> counts;
        const std::shared_ptr<std::unordered_map<char,size_t>> alphabet;
        const double alpha;

        size_t symbolPosition(char symbol) const {
            return alphabet->at(symbol);
        }

    public:
        ModelData(const std::shared_ptr<std::unordered_map<char,size_t>>& _alphabet,const double _alpha): alphabet(_alphabet),alpha(_alpha) {}

        void increment(const std::string& context,char symbol) {
            if(counts.find(context) == counts.end()) {
                counts[context] = std::vector<size_t>(alphabet->size(),0);
            }
            counts[context][symbolPosition(symbol)]++;
        }

        virtual double estimateProbability(char symbol,const std::string& context) {
            if(counts.find(context) == counts.end())
                return alpha;

            std::vector<size_t>& data = counts.at(context);
            size_t symbolCount = data.at(symbolPosition(symbol));
            size_t sum = accumulate(data.begin(),data.end(),0);

            return (symbolCount + alpha) / (sum + alpha * alphabet->size());
        }
};

class MarkovModel {
    private:
        size_t contextSize = 1;
        char mostFrequent;
        std::unordered_map<char,size_t> counts;
    protected:
        ModelData modelData;

        virtual char findMostFrequent() {
            return max_element(counts.begin(),counts.end(),[](const std::pair<char,size_t>& a,const std::pair<char,size_t>& b) {
                    return a.second < b.second;
                })->first;
        }
    public:
        virtual void train(const std::string& data) {
            for(size_t i = 0; i < data.size() - contextSize; i++) {
                std::string context = data.substr(i,contextSize);
                char symbol = data[i + contextSize];
                modelData.increment(context,symbol);
                counts[symbol]++;
            }

            mostFrequent = findMostFrequent();
        }

        virtual double calculateBits(const std::string& text) {
            std::string extraBits(contextSize,mostFrequent);
            double bits = 0;
            std::string context(contextSize,mostFrequent);
            for(size_t i = 0; i < text.size(); i++) {
                char symbol = text[i];
                double prob = modelData.estimateProbability(symbol,context);
                bits += -log2(prob);
                context = context.substr(1) + symbol;
            }

            return bits;
        }

        MarkovModel(size_t _contextSize,const double _alpha,const std::shared_ptr<std::unordered_map<char,size_t>>& _alphabet): contextSize(_contextSize),modelData(_alphabet,_alpha) {}
        virtual ~MarkovModel() {}
};

bool isValidFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return false;
    }
    file.close();
    return true;
}

std::unordered_map<char,size_t> alphabet(const std::string& inputFilename) {
    std::ifstream inputFile(inputFilename);

    std::unordered_map<char,size_t> alphabet;

    char symbol;
    size_t counter = 0;

    while (inputFile.get(symbol)) {
        // Verificar se o caractere já está no alfabeto
        if (alphabet.find(symbol) == alphabet.end()) {
            // Se não estiver, adicioná-lo ao alfabeto
            alphabet[symbol] = counter;
            counter++;
        }
    }

    inputFile.close();
    return alphabet;
}

MarkovModel FCM(const std::string& filename, size_t k, const std::shared_ptr<std::unordered_map<char,size_t>>& alphabet) {
    #define MAX_BUFFER_SIZE 8000

    MarkovModel model(k,alpha,alphabet);
    std::ifstream file(filename);

    char* charBuffer = new char[MAX_BUFFER_SIZE];
    std::string data;
    
    while (file.readsome(charBuffer,MAX_BUFFER_SIZE)) {
        data.assign(charBuffer,file.gcount());
        model.train(data);
    }

    delete charBuffer;
    
    file.close();
    return model;
}

int main(int argc,char* argv[]) {
    /*argumentos:
        1 para a coleção de textos não reescrito pelo gpt
        2 para a coleção de textos reescritos pelo gpt
        3 texto em analise
        Ate agora temos estes 3
        Futuramente:
        4 tamanho da sliding window k
        5 alfa parametro de suavização
        ....
    */
    auto start = std::chrono::high_resolution_clock::now(); 
    
    if (argc != 4) {
        std::cerr << "Usage: "<< argv[0] << "<human_collection> <gpt_collection> <text_to_analyse>" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string humanCollectionFile = argv[1];
    std::string gptCollectionFile = argv[2];
    std::string textFile = argv[3];

    // Pode se especificar qual/quais tao incorretos
    // Verificar se os ficheiros de textos fornecidos são válidos
    std::cout << "Checking files..." << std::endl;
    if (!isValidFile(humanCollectionFile) || !isValidFile(gptCollectionFile) || !isValidFile(textFile)) {
        std::cerr << "Invalid file(s) provided!" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    // Obter o alfabeto do ficheiro de entrada
    std::cout << "Obtaining alphabet..." << std::endl;
    std::shared_ptr<std::unordered_map<char,size_t>> alphabetHuman = std::make_shared<std::unordered_map<char,size_t>>(alphabet(humanCollectionFile));
    std::shared_ptr<std::unordered_map<char,size_t>> alphabetGpt = std::make_shared<std::unordered_map<char,size_t>>(alphabet(gptCollectionFile));
    
    std::cout << "Applying FCM..." << std::endl;
    MarkovModel resultsHuman = FCM(humanCollectionFile, k, alphabetHuman);
    MarkovModel resultsGpt = FCM(gptCollectionFile, k, alphabetGpt);

    std::ifstream file(textFile);
    if(!file.is_open()) {
        std::cerr << "Error opening file: " << textFile << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string text(buffer.str());

    std::cout << "Determining class..." << std::endl;
    double bitsHuman = resultsHuman.calculateBits(text);
    double bitsGPT = resultsGpt.calculateBits(text);
    std::string res;

    std::cout << "Human score: " << bitsHuman << std::endl;
    std::cout << "GPT score: " << bitsGPT << std::endl;

    if(bitsHuman < bitsGPT)
        res = "Human";
    else
        res = "GPT";

    std::cout << "The text belongs to the class: " << res << std::endl;

    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
    std::cout << "Elapsed time: " << elapsed << " seconds" << std::endl;

    return 0;
}