#ifndef EVENTEMITTER_H
#define EVENTEMITTER_H

class EventEmitter {
private:
    std::vector<testIEventListener*> listeners;

public:
    // Добавляем слушателя
    void addListener(testIEventListener* listener) {
        listeners.push_back(listener);
    }

    // Уведомляем всех слушателей о событии
    void emitEvent(const std::string& event, DynamicJsonDocument doc) {
        if(event == "button_event"){
            for (testIEventListener* listener : listeners) {
                listener->onButtonAction(event, doc);
            }
            }
        else if(event == "timed_mode"){
            for (testIEventListener* listener : listeners) {
                listener->onTimedMode(event, doc);
            }
        }
    }
};

#endif