#include <iostream>
#include <string>
#include <vector>

// Интерфейс наблюдателя
class IEventListener {
public:
    virtual void onEvent(const std::string& event) = 0;
    virtual ~IEventListener() = default;
};

// Класс root (система с насосами и клапанами)
class Root : public IEventListener {
public:
    void onEvent(const std::string& event) override {
        // Обработка события
        std::cout << "Root: Обновление компонентов на событие: " << event << std::endl;
        // Логика обновления компонентов (насосов и клапанов)
        updateComponents(event);
    }

    void updateComponents(const std::string& event) {
        // Пример обновления насосов и клапанов на основе события
        if (event == "button_click") {
            std::cout << "Насос включён/выключён!" << std::endl;
        } else if (event == "valve_open") {
            std::cout << "Клапан открыт!" << std::endl;
        } else if (event == "valve_close") {
            std::cout << "Клапан закрыт!" << std::endl;
        }
    }
};

// Класс события (источник событий)
class EventEmitter {
private:
    std::vector<IEventListener*> listeners;

public:
    // Добавляем слушателя
    void addListener(IEventListener* listener) {
        listeners.push_back(listener);
    }

    // Уведомляем всех слушателей о событии
    void emitEvent(const std::string& event) {
        for (IEventListener* listener : listeners) {
            listener->onEvent(event);
        }
    }
};

// Пример использования
int main() {
    // Создаём экземпляры классов
    Root rootSystem;
    EventEmitter eventEmitter;

    // Регистрируем root в качестве слушателя событий
    eventEmitter.addListener(&rootSystem);

    // Эмулируем приход событий от socket.io
    eventEmitter.emitEvent("button_click");  // Вывод: Насос включён/выключён!
    eventEmitter.emitEvent("valve_open");    // Вывод: Клапан открыт!
    eventEmitter.emitEvent("valve_close");   // Вывод: Клапан закрыт!

    return 0;
}
