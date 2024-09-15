
const express = require('express');
const fs = require('fs');
const app = express();
const PORT = 8080;

app.use(express.json());

app.listen(
    PORT,
    () => console.log("App is listening on PORT:" + PORT)
)

app.get('/api', (req, res) => {
    res.status(200).send({
        temp:10,
        hum:50
    })
})

app.post('/api/sendCurrentState', (req, res) => {
    const body = req.body;
    const newEntry = {
        timestamp: new Date().toISOString(),
        data: body
    };
    pushStatisticsToDB(body, newEntry);
    res.status(200).send({ message: "Data added successfully", newEntry });
})

let pushStatisticsToDB = (body, newEntry) => {
    if (!body || Object.keys(body).length === 0) {
        return res.status(400).send("ERROR: Body is missing");
    }

    // Читаем текущие данные из statistics.json
    fs.readFile('statistics.json', 'utf8', (err, data) => {
        if (err) {
            console.error("Error reading file:", err);
            return res.status(500).send("ERROR: Unable to read statistics.json");
        }

        let statistics = [];

        // Парсим содержимое файла или создаём пустой массив, если файл пуст
        try {
            statistics = JSON.parse(data) || [];
            console.log(statistics);
        } catch (parseError) {
            console.error("Error parsing JSON:", parseError);
            return res.status(500).send("ERROR: Invalid JSON format in statistics.json");
        }

        // Добавляем новую запись в массив статистики
        console.log(newEntry);
        statistics.push(newEntry);

        // Записываем обновлённые данные обратно в statistics.json
        fs.writeFile('statistics.json', JSON.stringify(statistics, null, 2), (writeErr) => {
            if (writeErr) {
                console.error("Error writing file:", writeErr);
                return res.status(500).send("ERROR: Unable to write to statistics.json");
            }
        });
    })
}