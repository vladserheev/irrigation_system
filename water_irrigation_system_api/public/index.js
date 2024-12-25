localStorage.debug = '*';

let sensorsSet = {
    DHT22Sensor: {
        temperature: {
            title: "Temperature",
            units: "°C"
        },
        humidity: {
            title: "Humidity",
            units: "%"
        }
    },
    SoilMoistureSensor: {
        title: "Soil Moisture",
        units: "%"
    }
}

// updating weather

const apiKey = "keE2CKNR5DZqtpC6CezHNEiF8UskF4LL"; // Replace with your AccuWeather API key
const city = "Lublin"; // City name for the query
const citySearchUrl = `https://dataservice.accuweather.com/locations/v1/cities/search?apikey=${apiKey}&q=${city}&language=pl-pl`;
const weatherBaseUrl = `https://dataservice.accuweather.com/currentconditions/v1/`;

async function fetchWeather() {
    try {
        // Step 1: Get City Key
        const cityResponse = await fetch(citySearchUrl);
        if (!cityResponse.ok) {
            throw new Error("Unable to fetch city data");
        }
        const cityData = await cityResponse.json();
        const cityKey = cityData[0]?.Key;

        if (!cityKey) {
            throw new Error("City not found");
        }

        // Step 2: Get Weather Data
        const weatherUrl = `${weatherBaseUrl}${cityKey}?apikey=${apiKey}&language=pl-pl&details=true`;
        const weatherResponse = await fetch(weatherUrl);
        if (!weatherResponse.ok) {
            throw new Error("Unable to fetch weather data");
        }
        const weatherData = await weatherResponse.json();
        console.log(weatherData);
        const currentWeather = weatherData[0];

        // Extract weather details
        const temperature = Math.round(currentWeather.Temperature.Metric.Value);
        const description = currentWeather.WeatherText;
        const iconNumber = currentWeather.WeatherIcon;

        // Update DOM
        document.getElementById("temperature").textContent = `${temperature}°C`;
        document.getElementById("weatherDescription").textContent = description;
        document.getElementById("weatherIcon").textContent = getWeatherEmoji(iconNumber);

    } catch (error) {
        console.error("Error fetching weather:", error);
    }
}

function getWeatherEmoji(iconNumber) {
    // Map AccuWeather icon numbers to emojis
    console.log(iconNumber, "icon number");
    const emojiMap = {
        1: "☀️",  // Sunny
        2: "🌤️", // Mostly Sunny
        3: "⛅",  // Partly Sunny
        4: "🌥️", // Intermittent Clouds
        5: "🌥️", // Hazy Sunshine
        6: "🌫️", // Mostly Cloudy
        7: "☁️",  // Cloudy
        8: "☁️",  // Dreary
        11: "🌫️", // Fog
        12: "🌦️", // Showers
        13: "🌦️", // Mostly Cloudy with Showers
        14: "🌧️", // Partly Sunny with Showers
        15: "⛈️", // Thunderstorms
        16: "⛈️", // Mostly Cloudy with Thunderstorms
        17: "⛈️", // Partly Sunny with Thunderstorms
        18: "🌧️", // Rain
        19: "🌨️", // Flurries
        20: "🌨️", // Mostly Cloudy with Flurries
        21: "🌨️", // Partly Sunny with Flurries
        22: "❄️",  // Snow
        23: "❄️",  // Mostly Cloudy with Snow
        24: "🌨️", // Ice
        25: "🌨️", // Sleet
        26: "🌧️", // Freezing Rain
        29: "🌧️", // Rain and Snow
        30: "🌬️", // Hot
        31: "🌡️", // Cold
        32: "🌬️", // Windy
        33: "🌙",  // Clear (Night)
        34: "🌤️", // Mostly Clear (Night)
        35: "⛅",  // Partly Cloudy (Night)
        36: "🌥️", // Intermittent Clouds (Night)
        37: "🌥️", // Hazy Moonlight (Night)
        38: "☁️",  // Mostly Cloudy (Night)
        39: "🌦️", // Partly Cloudy with Showers (Night)
        40: "🌧️", // Mostly Cloudy with Showers (Night)
        41: "⛈️", // Partly Cloudy with Thunderstorms (Night)
        42: "⛈️", // Mostly Cloudy with Thunderstorms (Night)
        43: "🌨️", // Mostly Cloudy with Flurries (Night)
        44: "❄️"
    };
    return emojiMap[iconNumber] || "🌈";
}


document.addEventListener('DOMContentLoaded', () => {
    const mobileMenuBtn = document.getElementById('mobileMenuBtn');
    const navMenu = document.getElementById('navMenu');
    const navButtons = document.querySelectorAll('.nav-button');

    // Toggle mobile menu visibility
    mobileMenuBtn.addEventListener('click', () => {
        if (navMenu.style.maxHeight === '0px' || !navMenu.style.maxHeight) {
            navMenu.style.maxHeight = navMenu.scrollHeight + 'px';
        } else {
            navMenu.style.maxHeight = '0px';
        }
    });

    // Handle active state for buttons
    navButtons.forEach(button => {
        button.addEventListener('click', () => {
            navButtons.forEach(btn => btn.classList.remove('active'));
            button.classList.add('active');

            // Scroll to the target section
            const target = document.querySelector(button.dataset.target);
            if (target) {
                target.scrollIntoView({ behavior: 'smooth' });
            }
        });
    });
});


$(document).ready(function () {
    fetchWeather();
    // Initially show the Direct Control section
    $('#mainSection').show();
    //alert();
    $("#dateRangeInput").val(new Date().toISOString().substring(0, 10));

    $('.nav-card').not('#mainSection').hide();

    // Add click event to navigation buttons
    $('.nav-button').on('click', function () {
        $('.nav-button').removeClass('active');
        $('.nav-card').hide();
        $(this).addClass('active');
        const target = $(this).data('target');
        $(target).show();
    });

    $('.add-schedule').click(function () {
        const zoneName = $(this).parent().attr("id");
        addSchedule(zoneName, "", "");
    });

    $(document).on('click', '.delete-schedule', function() {
        $(this).closest('.schedule-block').fadeOut(200, function() {
            $(this).remove();
        });
    });

    // Update display values for range inputs
    $('.range-input').on('input', function () {
        const displayId = $(this).data('display');
        const displayValue = $(`span[data-display="${displayId}"]`);
        displayValue.text($(this).val());
    });

    // Socket connection setup
    const socket = io.connect();

    socket.on("connect", () => {
        console.log("Connected to the server!");
        socket.emit("client", true); // Notify server of client connection

    });

    // Listen for system state updates
    socket.on('sendCurrentState', (data) => {
        console.log('Received initial system data:', data);
        updateUI(data);

    });

    socket.on("timedModeSettings", (zones) => {
        console.log("Received timed mode settings");
        console.log(zones.zones);
        zones.zones.forEach(zone => {
            const zoneName = zone.name.toLowerCase();
            console.log(zoneName);
            $(`#${zoneName} .schedules`).empty();
            zone.schedules.forEach(schedule => {
                //console.log(schedule.startHour.toString())
                const startVal = makeValidTime(schedule.startHour, schedule.startMinute);
                const endVal = makeValidTime(schedule.finishHour, schedule.finishMinute);
                addSchedule(zoneName, startVal, endVal)
            })
        })

        // zones.zones.forEach(zone => {
        //
        //     const zoneId = $(this).data('zone');
        //     const defaultStart = $(this).data('start') || '';
        //     const defaultEnd = $(this).data('end') || '';
        //     $(`#zone${zoneId} .schedules`).append(getScheduleTemplate(defaultStart, defaultEnd));
        // })
    })

    socket.on('updateCurrentStateOnClientSide', (set) => {
        console.log('Received updated system data:', set);
        updateUI(set);
    });

    socket.on("isConnectedToMaster", (status) => {
        console.log('Master connection status:', status);
        $('#connectionToMaster-status').text(status ? 'Połączono' : 'Odłączono');
        $('#connectionToMaster-status').addClass(status ? "text-green-500" : "text-red-400");

    });

    socket.on("getDataForStatistics", (data) => {
        if (data) {
            console.log("get data for statistics", data);
            $(window).trigger('dataReceived', [data]);
        }
    });

    // Handle disconnection
    socket.on('disconnect', () => {
        console.log('Disconnected from the server');
        $('#connectionToMaster-status').text('DISCONNECTED');
    });

    // Toggle buttons (ON/OFF)
    $('.toggle-button.control').each(function () {
        $(this).on('click', function () {
            const action = $(this).text() === 'Wł' ? 'OFF' : 'ON';
            //$(this).addClass(action ? 'OFF' : 'ON');

            socket.emit("btnAction", {btnName: this.id, action}, (response) => {
                console.log(response);
            });
            if(action === "OFF"){
                $(this).text("Wył");
                $(this).removeClass("active");
            }else{
                $(this).text("Wł");
                $(this).addClass("active");
            }

        });
    });

    // Handle form submissions
    $("#manualSettingsForm").submit((event) => {
        event.preventDefault(); // Prevent default form submission

        console.log("Form submission event:", event);

        // Convert form data to an object instead of a serialized string
        const formDataArray = $(event.currentTarget).serializeArray();
        const formDataObject = {};
        formDataArray.forEach(field => {
            formDataObject[field.name] = field.value;
        });
        console.log("Form data as object:", formDataObject);

        // Construct the dataToSend object
        let dataToSend = {
            zones: [
                {
                    name: "zone1",
                    settings: formDataObject // Send as an object
                }
            ]
        };
        console.log("Data to send:", dataToSend);

        // Validate the object data
        if (manualSettingsFormValidate(formDataObject)) {
            // Send AJAX request
            $.ajax({
                type: "POST",
                url: "/api/manualSettingsForm",
                data: JSON.stringify(dataToSend), // Serialize data as JSON
                contentType: "application/json", // Ensure server knows we are sending JSON
                success: (response) => {
                    console.log("Server response:", response);
                },
                error: (xhr, status, error) => {
                    console.error("AJAX error:", status, error);
                    alert(`Error: ${status} - ${error}`);
                }
            });
        } else {
            console.warn("Form validation failed.");
        }
    });


    $("#timeSettingsForm").submit((event) => {
        event.preventDefault();
        const schedules = [];
        // Loop through the zones to gather schedules
        let zoneNumber = 1;
        $('.zoneTimedMode').each(function () {
            //console.log("hello");
            const zoneSchedules = [];
            $(this).find(".schedule").each(function () {
                    const startTime = $(this).find(".start").val().split(':');
                    const endTime = $(this).find(".end").val().split(':');

                    zoneSchedules.push({
                        startHour: parseInt(startTime[0]),
                        startMinute: parseInt(startTime[1]),
                        finishHour: parseInt(endTime[0]),
                        finishMinute: parseInt(endTime[1])
                    });
                }
            )
            if (zoneSchedules) {
                schedules.push({
                    name: `zone${zoneNumber}`,
                    schedules: zoneSchedules
                });
            }

            zoneNumber++;
        });
        console.log(schedules);
        //Prepare data for sending
        const dataToSend = {zones: schedules};
        console.log(dataToSend);

        //AJAX request
        $.ajax({
            type: "POST",
            url: "/api/timeSettingsForm",
            contentType: "application/json",
            data: JSON.stringify(dataToSend),
            success: (response) => console.log("Server response:", response),
            error: (xhr, status, error) => alert(`Error: ${status} - ${error}`)
        });
    });
});

// Validate forms (expand these as needed)
const timeSettingsFormValidate = (data) => {
    // Add specific validation logic
    return true;
};

const manualSettingsFormValidate = (data) => {
    // Add specific validation logic
    return true;
};

// Function to update the UI based on the system state
function updateUI(data) {
    const pump = data.System.Components.Pump;
    const valves = data.System.Components.Valves;
    const sensors = data.System.Sensors;

    // Update mode
    $("#currentMode").text(data.System.Mode);

    // Clear existing component and sensor elements
    $(".components").empty();
    $(".sensors").empty();

    // Update pump status
    $(".components").append(`<p>${pump.name} Status: <span>${pump.status ? 'ON' : 'OFF'}</span></p>`);

    // Update the toggled button for the pump (assuming button ID matches the pump name or another identifier)
    const pumpButton = $(`#${pump.name}`);  // Assuming pump button has an id matching its name
    console.log(pumpButton);
    if (pumpButton.length) {
        pumpButton.text(pump.status ? 'ON' : 'OFF');  // Set text based on status
    }

    // Update valves status
    valves.forEach((valve) => {
        $(".components").append(`<p>${valve.name} Status: <span>${valve.status ? 'ON' : 'OFF'}</span></p>`);

        // Update the toggled button for the valve
        const valveButton = $(`#${valve.name}`);
        console.log(valve.name);
        console.log(valveButton);// Assuming each valve has an associated button with an id matching the valve name
        if (valveButton.length) {
            valveButton.text(valve.status ? 'ON' : 'OFF');  // Set text based on status
        }
    });
    //console.log($('#Valve_3'));
    // Update sensors
    sensors.forEach((sensor) => {
        if (sensor.sensorName === "DHT22Sensor") {
            $(".sensors #TemperatureSensorOutput").val(sensor.value, sensorsSet[sensor.sensorName].temperature.units);
            $(".sensors #AirHumiditySensorOutput").val(sensor.value, sensorsSet[sensor.sensorName].humidity.units);
            // $(".sensors").append(`<p>${sensorsSet[sensor.sensorName].temperature.title}: <span>${sensor.tempValue} ${sensorsSet[sensor.sensorName].temperature.units}</span></p>`);
            // $(".sensors").append(`<p>${sensorsSet[sensor.sensorName].humidity.title}: <span>${sensor.humValue} ${sensorsSet[sensor.sensorName].humidity.units}</span></p>`);
        } else if (sensor.sensorName === "SoilMoistureSensor"){
            $(".sensors #SoilMoistureSensorOutput").val(sensor.value, sensorsSet[sensor.sensorName].units);
            //$(".sensors").append(`<p>${sensorsSet[sensor.sensorName].title}: <span>${sensor.value} ${sensorsSet[sensor.sensorName].units}</span></p>`);
        }

    });
}

const addSchedule = (zoneName, startVal, endVal) => {
    //const zone = $(this).data('zone');
    //zoneName = "zone1";
    //console.log(zoneName);
    const zoneNumber = zoneName.split(" ")[1];
    $(`#${zoneName} .schedules`).append(getScheduleTemplate(startVal, endVal));
    //$(`#${zoneName} .schedules`).append(scheduleHTML);
}

let makeValidTime = (hours, minutes) => {
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


// preloader

document.addEventListener('DOMContentLoaded', function() {
    const preloader = document.getElementById('preloader');
    const content = document.querySelector('.content');

    // Имитация минимального времени загрузки (0.5 секунды)
    setTimeout(() => {
        preloader.classList.add('fade-out');
        content.classList.add('visible');

        // Удаляем preloader после анимации
        setTimeout(() => {
            preloader.style.display = 'none';
        }, 500);
    }, 500);
});



function getScheduleTemplate(defaultStart = '', defaultEnd = '') {
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
                        <input type="time" name="start_time[]" value="${defaultStart}" class="form-input rounded-md border-gray-200 focus:border-blue-500 focus:ring focus:ring-blue-200 transition-colors">
                    </div>
                    <div class="flex flex-col">
                        <label class="text-sm font-medium text-gray-600 mb-1">End</label>
                        <input type="time" name="end_time[]" value="${defaultEnd}" class="form-input rounded-md border-gray-200 focus:border-blue-500 focus:ring focus:ring-blue-200 transition-colors">
                    </div>
                </div>
            </div>
        `;
}