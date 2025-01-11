localStorage.debug = '*';

const CONFIG = {
    DEBUG: '*',
    WEATHER_API: {
        KEY: "keE2CKNR5DZqtpC6CezHNEiF8UskF4LL",
        CITY: "Lublin",
        BASE_URL: "https://dataservice.accuweather.com",
        LANGUAGE: "pl-pl"
    },
    SENSORS: {
        DHT22Sensor: {
            temperature: { title: "Temperature", units: "°C" },
            humidity: { title: "Humidity", units: "%" }
        },
        SoilMoistureSensor: {
            title: "Soil Moisture",
            units: "%"
        }
    },
    preloaderTime: 200
};

class WeatherService {
    static async fetchWeather() {
        try {
            const cityKey = await this.getCityKey();
            const weatherData = await this.getWeatherData(cityKey);
            this.updateWeatherUI(weatherData[0]);
        } catch (error) {
            console.error("Error fetching weather:", error);
        }
    }

    static async getCityKey() {
        const citySearchUrl = `${CONFIG.WEATHER_API.BASE_URL}/locations/v1/cities/search?apikey=${CONFIG.WEATHER_API.KEY}&q=${CONFIG.WEATHER_API.CITY}&language=${CONFIG.WEATHER_API.LANGUAGE}`;
        const response = await fetch(citySearchUrl);
        if (!response.ok) throw new Error("Unable to fetch city data");

        const cityData = await response.json();
        const cityKey = cityData[0]?.Key;
        if (!cityKey) throw new Error("City not found");

        return cityKey;
    }

    static async getWeatherData(cityKey) {
        const weatherUrl = `${CONFIG.WEATHER_API.BASE_URL}/currentconditions/v1/${cityKey}?apikey=${CONFIG.WEATHER_API.KEY}&language=${CONFIG.WEATHER_API.LANGUAGE}&details=true`;
        const response = await fetch(weatherUrl);
        if (!response.ok) throw new Error("Unable to fetch weather data");
        return response.json();
    }

    static updateWeatherUI(weatherData) {
        const temperature = Math.round(weatherData.Temperature.Metric.Value);
        document.getElementById("temperature").textContent = `${temperature}°C`;
        document.getElementById("weatherDescription").textContent = weatherData.WeatherText;
        document.getElementById("weatherIcon").textContent = WeatherEmojis.get(weatherData.WeatherIcon);
    }
}

// Weather Emojis Mapping
class WeatherEmojis {
    static #emojiMap = {
        1: "☀️", 2: "🌤️", 3: "⛅", 4: "🌥️", 5: "🌥️", 6: "🌫️", 7: "☁️", 8: "☁️",
        11: "🌫️", 12: "🌦️", 13: "🌦️", 14: "🌧️", 15: "⛈️", 16: "⛈️", 17: "⛈️",
        18: "🌧️", 19: "🌨️", 20: "🌨️", 21: "🌨️", 22: "❄️", 23: "❄️", 24: "🌨️",
        25: "🌨️", 26: "🌧️", 29: "🌧️", 30: "🌬️", 31: "🌡️", 32: "🌬️", 33: "🌙",
        34: "🌤️", 35: "⛅", 36: "🌥️", 37: "🌥️", 38: "☁️", 39: "🌦️", 40: "🌧️",
        41: "⛈️", 42: "⛈️", 43: "🌨️", 44: "❄️"
    };

    static get(iconNumber) {
        return this.#emojiMap[iconNumber] || "🌈";
    }
}

class UIController {
    static initializeUI() {
        this.runPreloader();
        this.setupMobileMenu();
        this.setupNavigation();
        this.setupEventListeners();
        this.initializeDateInput();
    }

    static runPreloader(){
        const preloader = document.getElementById('preloader');
        const content = document.querySelector('.content');

        // Имитация минимального времени загрузки (0.5 секунды)
        setTimeout(() => {
            preloader.classList.add('fade-out');
            content.classList.add('visible');

            // Удаляем preloader после анимации
            setTimeout(() => {
                preloader.style.display = 'none';
            }, CONFIG.preloaderTime);
        }, CONFIG.preloaderTime);
    }

    static setupMobileMenu() {
        const mobileMenuBtn = document.getElementById('mobileMenuBtn');
        const navMenu = document.getElementById('navMenu');

        mobileMenuBtn?.addEventListener('click', () => {
            const isCollapsed = navMenu.style.maxHeight === '0px' || !navMenu.style.maxHeight;
            navMenu.style.maxHeight = isCollapsed ? `${navMenu.scrollHeight}px` : '0px';
        });
    }

    static setupNavigation() {
        $('.nav-card').not('#mainSection').hide();
        const navButtons = document.querySelectorAll('.nav-button');
        navButtons.forEach(button => {
            button.addEventListener('click', () => this.handleNavigation(button, navButtons));
        });
    }

    static handleNavigation(button, allButtons) {
        allButtons.forEach(btn => btn.classList.remove('active'));
        button.classList.add('active');
        $('.nav-card').hide();
        const target = document.querySelector(button.dataset.target);
        $(target).show();
        target?.scrollIntoView({ behavior: 'smooth' });
    }

    static setupEventListeners() {
        $('.range-input').on('input', this.handleRangeInput);
        $('.add-schedule').click(this.handleAddSchedule);
        $(".toggle-button:checkbox").change(this.handleControlButton);
        $(document).on('click', '.delete-schedule', this.handleDeleteSchedule);
    }

    static handleControlButton(){
        SocketManager.handleControlButton(this.id, this.checked ? 'ON' : 'OFF')
    }

    static initializeDateInput() {
        $("#dateRangeInput").val(new Date().toISOString().substring(0, 10));
    }

    static handleRangeInput() {
        const displayId = $(this).data('display');
        $(`span[data-display="${displayId}"]`).text($(this).val());
    }

    static handleAddSchedule() {
        const zoneName = $(this).parent().attr("id");
        console.log($(this).parent());
        ScheduleManager.addSchedule(zoneName);
    }

    static handleDeleteSchedule() {
        $(this).closest('.schedule-block').fadeOut(200, function() {
            $(this).remove();
        });
    }

    static updateUI(data) {
        const pump = data.System.Components.Pump;
        const valves = data.System.Components.Valves;
        const sensors = data.System.Sensors;

        // Update mode
        $("#currentMode").text(data.System.Mode);

        // Clear existing component and sensor elements
        // Update pump status

        // Update the toggled button for the pump (assuming button ID matches the pump name or another identifier)
        const pumpButton = $(`#${pump.name}`);  // Assuming pump button has an id matching its name
        console.log(pumpButton);
        if (pumpButton.length) {
            //updateButton(pumpButton, pump.status ? 'OFF' : 'ON');
            pumpButton.prop('checked', pump.status);
            //pumpButton.text();  // Set text based on status
        }

        // Update valves status
        valves.forEach((valve) => {
            // Update the toggled button for the valve
            const valveButton = $(`#${valve.name}`);
            console.log(valve.name);
            console.log(valveButton);// Assuming each valve has an associated button with an id matching the valve name
            if (valveButton.length) {
                valveButton.prop('checked', valve.status);
            }
        });

        sensors.forEach((sensor) => {
            if (sensor.sensorName === "DHT22Sensor") {
                console.log(CONFIG.SENSORS[sensor.sensorName]);
                $(".sensors #TemperatureSensorOutput").text(sensor.tempValue + CONFIG.SENSORS[sensor.sensorName].temperature.units);
                $(".sensors #AirHumiditySensorOutput").text(sensor.humValue + CONFIG.SENSORS[sensor.sensorName].humidity.units);
                console.log(sensor.humValue, CONFIG.SENSORS[sensor.sensorName].humidity.units);
            } else if (sensor.sensorName === "SoilMoistureSensor"){
                $(".sensors #SoilMoistureSensorOutput").text(sensor.value + CONFIG.SENSORS[sensor.sensorName].units);
                //$(".sensors").append(`<p>${sensorsSet[sensor.sensorName].title}: <span>${sensor.value} ${sensorsSet[sensor.sensorName].units}</span></p>`);
            }

        });
    }
}

class SocketManager {
    static initialize() {
        this.socket = io.connect();
        this.setupSocketListeners();
    }

    static setupSocketListeners() {
        this.socket.on("connect", () => {
            console.log("Connected to the server!");
            this.socket.emit("client", true);
        });

        this.socket.on('sendCurrentState', this.handleSystemState);
        this.socket.on('updateCurrentStateOnClientSide', this.handleSystemState);
        this.socket.on("isConnectedToMaster", this.handleMasterConnection);
        this.socket.on("getDataForStatistics", this.handleStatisticsData);
        this.socket.on("timedModeSettings", this.handleTimedModeSettings);
        this.socket.on('disconnect', this.handleDisconnect);
    }

    static handleSystemState(data) {
        console.log('Received system data:', data);
        UIController.updateUI(data);
    }

    static handleControlButton(name, action){
        this.socket.emit("btnAction", {btnName: name, action}, (response) => {
            console.log(response);
        });
    }

    static handleMasterConnection(status) {
        const statusElement = $('#connectionToMaster-status');
        statusElement.text(status ? 'Połączono' : 'Odłączono')
            .removeClass('text-green-500 text-red-400')
            .addClass(status ? "text-green-500" : "text-red-400");
    }

    static handleStatisticsData(data) {
        if (data) {
            console.log("Statistics data received:", data);
            $(window).trigger('dataReceived', [data]);
        }
    }

    static handleTimedModeSettings(data) {
        console.log("Received timed mode settings:", data);
        ScheduleManager.updateSchedules(data.zones);
    }

    static handleDisconnect() {
        console.log('Disconnected from the server');
        $('#connectionToMaster-status').text('DISCONNECTED');
    }
}

class FormManager {
    static initialize() {
        this.setupFormSubmissions();
    }

    static setupFormSubmissions() {
        $("#manualSettingsForm").submit(this.handleManualSettingsSubmit);
        $("#timeSettingsForm").submit(this.handleTimeSettingsSubmit);
    }

    static handleManualSettingsSubmit(event) {
        event.preventDefault();
        const formData = FormManager.getFormData($(event.currentTarget));

        if (FormManager.validateManualSettings(formData)) {
            FormManager.submitForm('/api/manualSettingsForm', {
                zones: [{ name: "zone1", settings: formData }]
            });
        }
    }

    static handleTimeSettingsSubmit(event) {
        event.preventDefault();
        const schedules = ScheduleManager.collectSchedules();
        FormManager.submitForm('/api/timeSettingsForm', { zones: schedules });
    }

    static getFormData($form) {
        const formDataArray = $form.serializeArray();
        const formData = {};
        formDataArray.forEach(field => formData[field.name] = field.value);
        return formData;
    }

    static submitForm(url, data) {
        $.ajax({
            type: "POST",
            url: url,
            contentType: "application/json",
            data: JSON.stringify(data),
            success: response => console.log("Server response:", response),
            error: (xhr, status, error) => console.error(`Error: ${status} - ${error}`)
        });
    }

    static validateManualSettings(data) {
        // Add validation logic here
        return true;
    }
}

class ScheduleManager  {
    static addSchedule(zoneName, startTime, finishTime){
        const zoneNumber = zoneName.split(" ")[1];
        $(`#${zoneName} .schedules-block`).append(this.getScheduleTemplate(startTime, finishTime));
    }

    static collectSchedules (){
        const schedules = [];
        let zoneNumber = 1;
        const formSelector = "#timeSettingsForm";
        // Find all zones with schedules
        $(formSelector).find('.zoneTimedMode').each(function() {
            console.log(this);
            const zoneSchedules = [];
            // Find all schedules within this zone
            $(this).find(".schedules-block").each(function() {
                console.log(this);
                $(this).find(".schedule-block").each(function (){
                    console.log("find");
                    const start = $(this).find('.start').val();
                    console.log(start);
                    const end = $(this).find('.end').val();

                    // Skip if times aren't properly set
                    if (!start || !end) return;

                    const [startHours, startMinutes] = start.split(':').map(Number);
                    const [endHours, endMinutes] = end.split(':').map(Number);

                    zoneSchedules.push({
                        startHour: startHours,
                        startMinute: startMinutes,
                        finishHour: endHours,
                        finishMinute: endMinutes
                    });
                })
            });

            // Only add zone if it has schedules
            if (zoneSchedules.length > 0) {
                schedules.push({
                    name: `zone${zoneNumber}`,
                    schedules: zoneSchedules
                });
            }

            zoneNumber++;
        });
        console.log(schedules);
        return schedules;
    }
    static getScheduleTemplate(defaultStart = '', defaultEnd = '') {
        return `
            <div class="schedule-block relative">
                <button type="button" class="delete-schedule absolute top-3 right-3 text-gray-400 hover:text-red-500 transition-colors">
                    <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12"></path>
                    </svg>
                </button>
                <div class="backgroung-white card grid grid-cols-2 gap-4 pr-8">
                    <div class="flex flex-col">
                        <label class="text-sm font-medium text-gray-600 mb-1">Start</label>
                        <input type="time" name="start" value="${defaultStart}" class="start form-input rounded-md border-gray-200 focus:border-blue-500 focus:ring focus:ring-blue-200 transition-colors" required>
                    </div>
                    <div class="flex flex-col">
                        <label class="text-sm font-medium text-gray-600 mb-1">End</label>
                        <input type="time" name="end" value="${defaultEnd}" class="end form-input rounded-md border-gray-200 focus:border-blue-500 focus:ring focus:ring-blue-200 transition-colors" required>
                    </div>
                </div>
            </div>
        `;
    }

    static updateSchedules(zones){
        console.log("Updating zone!!!!!");
        $(`.schedules-block`).empty();

        zones.forEach(zone => {
            const zoneName = zone.name.toLowerCase();
            console.log(zoneName);

           // $(`#${zoneName} .schedules`).empty();
            zone.schedules.forEach(schedule => {
                //console.log(schedule.startHour.toString())
                const startVal = this.makeValidTime(schedule.startHour, schedule.startMinute);
                const endVal = this.makeValidTime(schedule.finishHour, schedule.finishMinute);
                this.addSchedule(zoneName, startVal, endVal)
            })
        })
    }

    static makeValidTime(hours, minutes){
        //const validedTime = "";
        hours = hours.toString();
        minutes = minutes.toString();
        var newHours = "";
        var newMinutes = "";

        if (hours < 10) {
            newHours += "0";
        }
        if (minutes < 10) {
            newMinutes += "0";
        }
        //console.log(hours);
        //console.log(minutes);
        newHours += hours;
        newMinutes += minutes;
        return (newHours + ":" + newMinutes);
        }
    }

$(document).ready(function () {
    UIController.initializeUI();
    WeatherService.fetchWeather();
    SocketManager.initialize();
    FormManager.initialize();
});




