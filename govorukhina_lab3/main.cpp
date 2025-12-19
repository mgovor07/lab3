#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <limits>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <chrono>

using namespace std;
namespace fs = filesystem;

struct Pipe {
    int id;
    string name;
    double length;
    int diameter;
    bool underRepair;
};

struct CompressorStation {
    int id;
    string name;
    int totalWorkshops;
    int activeWorkshops;
    int stationClass;
};

class Logger {
private:
    mutable ofstream logFile;
    
public:
    Logger() {
        logFile.open("pipeline_log.txt", ios::app);
        if (logFile.is_open()) {
            auto now = chrono::system_clock::now();
            auto time = chrono::system_clock::to_time_t(now);
            logFile << "\n=== Сессия начата: " << ctime(&time);
        }
    }
    
    ~Logger() {
        if (logFile.is_open()) {
            auto now = chrono::system_clock::now();
            auto time = chrono::system_clock::to_time_t(now);
            logFile << "=== Сессия завершена: " << ctime(&time) << endl;
            logFile.close();
        }
    }
    
    void log(const string& action, const string& details = "") const {
        if (logFile.is_open()) {
            auto now = chrono::system_clock::now();
            auto time = chrono::system_clock::to_time_t(now);
            
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&time));
            
            logFile << timeStr << " | " << action;
            if (!details.empty()) {
                logFile << " | " << details;
            }
            logFile << endl;
        }
    }
};

class InputValidator {
public:
    static int getIntInput(const string& prompt, int min = numeric_limits<int>::min(),
                          int max = numeric_limits<int>::max()) {
        int value;
        while (true) {
            cout << prompt;
            string input;
            getline(cin, input);
            
            if (input.empty()) {
                cout << "Ошибка: ввод не может быть пустым.\n";
                continue;
            }
            
            try {
                value = stoi(input);
                if (value < min || value > max) {
                    cout << "Ошибка: значение должно быть от " << min << " до " << max << ".\n";
                    continue;
                }
                break;
            } catch (const exception&) {
                cout << "Ошибка: пожалуйста, введите целое число.\n";
            }
        }
        return value;
    }

    static double getDoubleInput(const string& prompt, double min = 0.0,
                               double max = numeric_limits<double>::max()) {
        double value;
        while (true) {
            cout << prompt;
            string input;
            getline(cin, input);
            
            if (input.empty()) {
                cout << "Ошибка: ввод не может быть пустым.\n";
                continue;
            }
            
            try {
                value = stod(input);
                if (value < min || value > max) {
                    cout << "Ошибка: значение должно быть от " << min << " до " << max << ".\n";
                    continue;
                }
                break;
            } catch (const exception&) {
                cout << "Ошибка: пожалуйста, введите число.\n";
            }
        }
        return value;
    }

    static string getStringInput(const string& prompt) {
        string input;
        while (true) {
            cout << prompt;
            getline(cin, input);
            if (!input.empty()) {
                break;
            }
            cout << "Ошибка: ввод не может быть пустым.\n";
        }
        return input;
    }
};

class PipelineSystem {
private:
    vector<Pipe> pipes;
    vector<CompressorStation> stations;
    int nextPipeId = 1;
    int nextStationId = 1;
    Logger logger;

    int findPipeIndexById(int id) const {
        auto it = find_if(pipes.begin(), pipes.end(),
                         [id](const Pipe& p) { return p.id == id; });
        return it != pipes.end() ? distance(pipes.begin(), it) : -1;
    }

    int findStationIndexById(int id) const {
        auto it = find_if(stations.begin(), stations.end(),
                         [id](const CompressorStation& s) { return s.id == id; });
        return it != stations.end() ? distance(stations.begin(), it) : -1;
    }

    vector<int> parseIndicesFromInput(const string& input, const vector<int>& validIds) const {
        if (input == "all" || input == "ALL") {
            vector<int> allIndices;
            for (int i = 0; i < validIds.size(); ++i) {
                allIndices.push_back(i);
            }
            return allIndices;
        }
        
        vector<int> indices;
        stringstream ss(input);
        string token;
        
        while (getline(ss, token, ',')) {
            try {
                int id = stoi(token);
                auto it = find(validIds.begin(), validIds.end(), id);
                if (it != validIds.end()) {
                    indices.push_back(distance(validIds.begin(), it));
                } else {
                    cout << "Предупреждение: ID " << id << " не существует.\n";
                }
            } catch (const exception&) {
                cout << "Предупреждение: '" << token << "' не является числом.\n";
            }
        }
        
        sort(indices.begin(), indices.end());
        indices.erase(unique(indices.begin(), indices.end()), indices.end());
        return indices;
    }

    vector<int> selectMultipleObjects(const vector<int>& validIds, const string& objectType) const {
        if (validIds.empty()) {
            cout << "Нет доступных " << objectType << "!\n";
            return {};
        }
        
        cout << "\nВыберите ID " << objectType << " через запятую или 'all' для всех: ";
        string input;
        getline(cin, input);
        
        return parseIndicesFromInput(input, validIds);
    }

    vector<int> getPipeIds() const {
        vector<int> ids;
        for (const auto& pipe : pipes) {
            ids.push_back(pipe.id);
        }
        return ids;
    }

    vector<int> getStationIds() const {
        vector<int> ids;
        for (const auto& station : stations) {
            ids.push_back(station.id);
        }
        return ids;
    }

    static string toLower(const string& str) {
        string result = str;
        transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    double calculateInactivePercent(const CompressorStation& station) const {
        return station.totalWorkshops > 0 ?
               100.0 * (station.totalWorkshops - station.activeWorkshops) / station.totalWorkshops : 0.0;
    }

    vector<int> findPipesByName(const string& searchName) const {
        vector<int> result;
        string searchLower = toLower(searchName);
        
        for (size_t i = 0; i < pipes.size(); ++i) {
            if (toLower(pipes[i].name).find(searchLower) != string::npos) {
                result.push_back(i);
            }
        }
        return result;
    }

    vector<int> findPipesByRepairStatus(bool repairStatus) const {
        vector<int> result;
        for (size_t i = 0; i < pipes.size(); ++i) {
            if (pipes[i].underRepair == repairStatus) {
                result.push_back(i);
            }
        }
        return result;
    }

    vector<int> findStationsByName(const string& searchName) const {
        vector<int> result;
        string searchLower = toLower(searchName);
        
        for (size_t i = 0; i < stations.size(); ++i) {
            if (toLower(stations[i].name).find(searchLower) != string::npos) {
                result.push_back(i);
            }
        }
        return result;
    }

    vector<int> findStationsByInactivePercent(double targetPercent, int comparisonType) const {
        vector<int> result;
        for (size_t i = 0; i < stations.size(); ++i) {
            double inactivePercent = calculateInactivePercent(stations[i]);
            bool match = false;
            
            switch (comparisonType) {
                case 1: match = (inactivePercent > targetPercent); break;
                case 2: match = (inactivePercent < targetPercent); break;
                case 3: match = (abs(inactivePercent - targetPercent) < 0.01); break;
            }
            
            if (match) {
                result.push_back(i);
            }
        }
        return result;
    }

    void displayObjects(const vector<int>& pipeIndices, const vector<int>& stationIndices) const {
        if (pipeIndices.empty() && stationIndices.empty()) {
            cout << "Нет объектов для отображения.\n";
            return;
        }
        
        if (!pipeIndices.empty()) {
            cout << "\nТрубы (" << pipeIndices.size() << ")\n";
            for (int index : pipeIndices) {
                const Pipe& pipe = pipes[index];
                cout << "ID: " << pipe.id << " | " << pipe.name
                     << ", Длина: " << pipe.length << " км"
                     << ", Диаметр: " << pipe.diameter << " мм"
                     << ", В ремонте: " << (pipe.underRepair ? "Да" : "Нет") << endl;
            }
        }

        if (!stationIndices.empty()) {
            cout << "\nКС (" << stationIndices.size() << ")\n";
            for (int index : stationIndices) {
                const CompressorStation& station = stations[index];
                double inactivePercent = calculateInactivePercent(station);
                cout << "ID: " << station.id << " | " << station.name
                     << ", Цехов: " << station.totalWorkshops
                     << ", Работает: " << station.activeWorkshops
                     << fixed << setprecision(1) << ", Незадействовано: " << inactivePercent << "%"
                     << ", Класс: " << station.stationClass << endl;
            }
        }
    }

public:
    void addPipe() {
        Pipe newPipe;
        newPipe.id = nextPipeId++;
        newPipe.name = InputValidator::getStringInput("Введите название трубы: ");
        newPipe.length = InputValidator::getDoubleInput("Введите длину трубы (км): ", 0.001);
        newPipe.diameter = InputValidator::getIntInput("Введите диаметр трубы (мм): ", 1);
        newPipe.underRepair = false;
        
        pipes.push_back(newPipe);
        cout << "Труба '" << newPipe.name << "' добавлена с ID: " << newPipe.id << "!\n";
        logger.log("Добавлена труба", "ID: " + to_string(newPipe.id) + ", Название: " + newPipe.name);
    }

    void addStation() {
        CompressorStation newStation;
        newStation.id = nextStationId++;
        newStation.name = InputValidator::getStringInput("Введите название КС: ");
        newStation.totalWorkshops = InputValidator::getIntInput("Введите количество цехов: ", 1);
        newStation.activeWorkshops = InputValidator::getIntInput("Введите работающих цехов: ",
                                                               0, newStation.totalWorkshops);
        newStation.stationClass = InputValidator::getIntInput("Введите класс станции: ", 1);
        
        stations.push_back(newStation);
        cout << "КС '" << newStation.name << "' добавлена с ID: " << newStation.id << "!\n";
        logger.log("Добавлена КС", "ID: " + to_string(newStation.id) + ", Название: " + newStation.name);
    }

    void addMultipleObjects(bool isPipe) {
        int count = InputValidator::getIntInput(isPipe ? "Сколько труб добавить? " : "Сколько КС добавить? ", 1, 100);
        for (int i = 0; i < count; ++i) {
            cout << "\n" << (isPipe ? "Добавление трубы " : "Добавление КС ") << (i + 1) << " из " << count << "\n";
            isPipe ? addPipe() : addStation();
        }
        cout << "Добавлено " << count << (isPipe ? " труб" : " КС") << ". Всего: " << (isPipe ? pipes.size() : stations.size()) << "\n";
    }

    void deleteObjects(bool isPipe) {
        vector<int> indices = isPipe ?
            selectMultipleObjects(getPipeIds(), "труб") :
            selectMultipleObjects(getStationIds(), "КС");
            
        if (indices.empty()) return;
        
        sort(indices.rbegin(), indices.rend());
        int count = 0;
        
        for (int index : indices) {
            if (isPipe) {
                cout << "Удалена труба: " << pipes[index].name << " (ID: " << pipes[index].id << ")\n";
                logger.log("Удалена труба", "ID: " + to_string(pipes[index].id) + ", Название: " + pipes[index].name);
                pipes.erase(pipes.begin() + index);
            } else {
                cout << "Удалена КС: " << stations[index].name << " (ID: " << stations[index].id << ")\n";
                logger.log("Удалена КС", "ID: " + to_string(stations[index].id) + ", Название: " + stations[index].name);
                stations.erase(stations.begin() + index);
            }
            count++;
        }
        
        cout << "Удалено " << count << (isPipe ? " труб" : " КС") << ". Осталось: " << (isPipe ? pipes.size() : stations.size()) << "\n";
    }

    void editPipe() {
        if (pipes.empty()) {
            cout << "Нет доступных труб!\n";
            return;
        }
        
        viewAll();
        int id = InputValidator::getIntInput("Введите ID трубы для редактирования: ", 1);
        int index = findPipeIndexById(id);
        
        if (index == -1) {
            cout << "Труба с ID " << id << " не найдена!\n";
            return;
        }
        
        cout << "Редактирование трубы ID: " << pipes[index].id << " - " << pipes[index].name << endl;
        cout << "1. Изменить статус ремонта\n2. Редактировать параметры\n";
        int choice = InputValidator::getIntInput("Выберите действие: ", 1, 2);
        
        if (choice == 1) {
            pipes[index].underRepair = !pipes[index].underRepair;
            string status = pipes[index].underRepair ? "В ремонте" : "Работает";
            cout << "Статус ремонта изменен на: " << status << endl;
            logger.log("Изменен статус трубы", "ID: " + to_string(pipes[index].id) + ", Статус: " + status);
        } else {
            pipes[index].name = InputValidator::getStringInput("Введите новое название трубы: ");
            pipes[index].length = InputValidator::getDoubleInput("Введите новую длину трубы (км): ", 0.001);
            pipes[index].diameter = InputValidator::getIntInput("Введите новый диаметр трубы (мм): ", 1);
            cout << "Параметры трубы обновлены!\n";
            logger.log("Обновлена труба", "ID: " + to_string(pipes[index].id) + ", Новое название: " + pipes[index].name);
        }
    }

    void editStation() {
        if (stations.empty()) {
            cout << "Нет доступных КС!\n";
            return;
        }
        
        viewAll();
        int id = InputValidator::getIntInput("Введите ID КС для редактирования: ", 1);
        int index = findStationIndexById(id);
        
        if (index == -1) {
            cout << "КС с ID " << id << " не найдена!\n";
            return;
        }
        
        cout << "Редактирование КС ID: " << stations[index].id << " - " << stations[index].name << endl;
        cout << "1. Запустить/остановить цех\n2. Редактировать параметры\n";
        int choice = InputValidator::getIntInput("Выберите действие: ", 1, 2);
        
        if (choice == 1) {
            cout << "Текущее состояние: " << stations[index].activeWorkshops
                 << "/" << stations[index].totalWorkshops << " цехов работает\n";
            cout << "1. Запустить цех\n2. Остановить цех\n";
            int action = InputValidator::getIntInput("Выберите действие: ", 1, 2);
            
            if (action == 1 && stations[index].activeWorkshops < stations[index].totalWorkshops) {
                stations[index].activeWorkshops++;
                cout << "Цех запущен! Работает цехов: " << stations[index].activeWorkshops << endl;
                logger.log("Запущен цех КС", "ID: " + to_string(stations[index].id) + ", Работает цехов: " + to_string(stations[index].activeWorkshops));
            } else if (action == 2 && stations[index].activeWorkshops > 0) {
                stations[index].activeWorkshops--;
                cout << "Цех остановлен! Работает цехов: " << stations[index].activeWorkshops << endl;
                logger.log("Остановлен цех КС", "ID: " + to_string(stations[index].id) + ", Работает цехов: " + to_string(stations[index].activeWorkshops));
            } else {
                cout << "Невозможно выполнить операцию!\n";
            }
        } else {
            stations[index].name = InputValidator::getStringInput("Введите новое название КС: ");
            int newTotal = InputValidator::getIntInput("Введите новое количество цехов: ", 1);
            
            if (newTotal < stations[index].activeWorkshops) {
                stations[index].activeWorkshops = newTotal;
            }
            stations[index].totalWorkshops = newTotal;
            stations[index].stationClass = InputValidator::getIntInput("Введите новый класс станции: ", 1);
            
            cout << "Параметры КС обновлены!\n";
            logger.log("Обновлена КС", "ID: " + to_string(stations[index].id) + ", Новое название: " + stations[index].name);
        }
    }

    void searchPipes() {
        if (pipes.empty()) {
            cout << "Нет доступных труб для поиска!\n";
            return;
        }
        
        cout << "\nПоиск труб\n";
        cout << "1. По названию\n";
        cout << "2. По признаку 'в ремонте'\n";
        int choice = InputValidator::getIntInput("Выберите тип поиска: ", 1, 2);
        
        vector<int> results;
        string searchDetails;
        
        if (choice == 1) {
            string searchName = InputValidator::getStringInput("Введите название для поиска: ");
            results = findPipesByName(searchName);
            searchDetails = "Поиск по названию: " + searchName;
        } else {
            cout << "1. Трубы в ремонте\n";
            cout << "2. Трубы не в ремонте\n";
            int repairChoice = InputValidator::getIntInput("Выберите статус: ", 1, 2);
            bool searchRepairStatus = (repairChoice == 1);
            results = findPipesByRepairStatus(searchRepairStatus);
            searchDetails = "Поиск по статусу ремонта: " + string(searchRepairStatus ? "в ремонте" : "не в ремонте");
        }
        
        displayObjects(results, {});
        logger.log("Поиск труб", searchDetails + ", Найдено: " + to_string(results.size()));
    }

    void searchStations() {
        if (stations.empty()) {
            cout << "Нет доступных КС для поиска!\n";
            return;
        }
        
        cout << "\nПоиск КС\n";
        cout << "1. По названию\n";
        cout << "2. По проценту незадействованных цехов\n";
        int choice = InputValidator::getIntInput("Выберите тип поиска: ", 1, 2);
        
        vector<int> results;
        string searchDetails;
        
        if (choice == 1) {
            string searchName = InputValidator::getStringInput("Введите название для поиска: ");
            results = findStationsByName(searchName);
            searchDetails = "Поиск по названию: " + searchName;
        } else {
            cout << "1. КС с процентом незадействованных цехов БОЛЬШЕ заданного\n";
            cout << "2. КС с процентом незадействованных цехов МЕНЬШЕ заданного\n";
            cout << "3. КС с процентом незадействованных цехов РАВНЫМ заданному\n";
            int percentChoice = InputValidator::getIntInput("Выберите тип сравнения: ", 1, 3);
            double targetPercent = InputValidator::getDoubleInput("Введите процент незадействованных цехов (0-100): ", 0, 100);
            results = findStationsByInactivePercent(targetPercent, percentChoice);
            searchDetails = "Поиск по проценту: " + to_string(targetPercent) + "%, Тип: " + to_string(percentChoice);
        }
        
        displayObjects({}, results);
        logger.log("Поиск КС", searchDetails + ", Найдено: " + to_string(results.size()));
    }

    void viewAll() const {
        vector<int> allPipeIndices, allStationIndices;
        for (int i = 0; i < pipes.size(); ++i) allPipeIndices.push_back(i);
        for (int i = 0; i < stations.size(); ++i) allStationIndices.push_back(i);
        displayObjects(allPipeIndices, allStationIndices);
    }

    void saveData() {
        string filename = InputValidator::getStringInput("Введите имя файла для сохранения: ");
        if (filename.find('.') == string::npos) {
            filename += ".txt";
        }
        
        ofstream file(filename);
        if (!file.is_open()) {
            cout << "Ошибка: невозможно создать файл " << filename << endl;
            return;
        }
        
        file << "NEXT_PIPE_ID " << nextPipeId << endl;
        file << "NEXT_STATION_ID " << nextStationId << endl;
        
        file << "PIPES " << pipes.size() << endl;
        for (const auto& pipe : pipes) {
            file << pipe.id << endl << pipe.name << endl << pipe.length << endl
                 << pipe.diameter << endl << pipe.underRepair << endl;
        }
        
        file << "STATIONS " << stations.size() << endl;
        for (const auto& station : stations) {
            file << station.id << endl << station.name << endl << station.totalWorkshops << endl
                 << station.activeWorkshops << endl << station.stationClass << endl;
        }
        
        file.close();
        cout << "Данные сохранены в файл: " << fs::absolute(filename) << endl;
        logger.log("Сохранение данных", "Файл: " + filename + ", Трубы: " + to_string(pipes.size()) + ", КС: " + to_string(stations.size()));
    }

    void loadData() {
        string filename = InputValidator::getStringInput("Введите имя файла для загрузки: ");
        
        ifstream file(filename);
        if (!file.is_open()) {
            cout << "Ошибка: файл " << filename << " не найден.\n";
            return;
        }
        
        pipes.clear();
        stations.clear();
        
        string header;
        size_t count;
        
        file >> header >> nextPipeId;
        if (header != "NEXT_PIPE_ID") {
            file.seekg(0);
            nextPipeId = 1;
            nextStationId = 1;
        } else {
            file >> header >> nextStationId;
        }
        
        file >> header >> count;
        if (header != "PIPES") {
            cout << "Ошибка: неверный формат файла.\n";
            return;
        }
        file.ignore();
        
        for (size_t i = 0; i < count; ++i) {
            Pipe pipe;
            file >> pipe.id;
            file.ignore();
            getline(file, pipe.name);
            file >> pipe.length >> pipe.diameter >> pipe.underRepair;
            file.ignore();
            pipes.push_back(pipe);
        }
        
        file >> header >> count;
        if (header != "STATIONS") {
            cout << "Ошибка: неверный формат файла.\n";
            return;
        }
        file.ignore();
        
        for (size_t i = 0; i < count; ++i) {
            CompressorStation station;
            file >> station.id;
            file.ignore();
            getline(file, station.name);
            file >> station.totalWorkshops >> station.activeWorkshops >> station.stationClass;
            file.ignore();
            
            if (station.activeWorkshops > station.totalWorkshops) {
                station.activeWorkshops = station.totalWorkshops;
            }
            
            stations.push_back(station);
        }
        
        file.close();
        cout << "Данные загружены из файла: " << fs::absolute(filename) << endl;
        cout << "Загружено труб: " << pipes.size() << ", КС: " << stations.size() << endl;
        logger.log("Загрузка данных", "Файл: " + filename + ", Трубы: " + to_string(pipes.size()) + ", КС: " + to_string(stations.size()));
    }

    void run() {
        logger.log("Запуск программы");
        
        while (true) {
            cout << "\nСистема управления трубопроводом\n"
                 << "1. Добавить трубу\n2. Добавить КС\n3. Добавить несколько труб\n4. Добавить несколько КС\n"
                 << "5. Просмотр всех объектов\n6. Редактировать трубу\n7. Редактировать КС\n"
                 << "8. Удалить трубу\n9. Удалить КС\n10. Удалить несколько труб\n11. Удалить несколько КС\n"
                 << "12. Поиск труб\n13. Поиск КС\n14. Сохранить данные\n15. Загрузить данные\n0. Выход\n";
            
            int choice = InputValidator::getIntInput("Выберите действие: ", 0, 15);
            logger.log("Выбор меню", "Действие: " + to_string(choice));
            
            switch (choice) {
                case 1: addPipe(); break;
                case 2: addStation(); break;
                case 3: addMultipleObjects(true); break;
                case 4: addMultipleObjects(false); break;
                case 5: viewAll(); break;
                case 6: editPipe(); break;
                case 7: editStation(); break;
                case 8: deleteObjects(true); break;
                case 9: deleteObjects(false); break;
                case 10: deleteObjects(true); break;
                case 11: deleteObjects(false); break;
                case 12: searchPipes(); break;
                case 13: searchStations(); break;
                case 14: saveData(); break;
                case 15: loadData(); break;
                case 0:
                    cout << "Выход из программы.\n";
                    logger.log("Выход из программы");
                    return;
            }
        }
    }
};

int main() {
    PipelineSystem system;
    system.run();
    return 0;
}
