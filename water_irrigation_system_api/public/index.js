localStorage.debug = '*';
$(document).ready(function () {
    // Initially show the Direct Control section
    $('#directControl').show();
    $('.status').not('#directControl').hide();

    // Add click event to navigation buttons
    $('.nav-button').on('click', function () {
        $('.nav-button').removeClass('active');
        $('.status').hide();
        $(this).addClass('active');
        const target = $(this).data('target');
        $(target).show();
    });

    $('.add-schedule').click(function () {
        const zoneName=$(this).parent().attr("id");
        addSchedule(zoneName,"", "");
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
            zone.schedules.forEach(schedule => {
                //console.log(schedule.startHour.toString())
                const startVal = makeValidTime(schedule.startHour, schedule.startMinute);
                const endVal = makeValidTime(schedule.finishHour, schedule.finishMinute);
                addSchedule(zoneName, startVal, endVal)
            })
        })
    })

    socket.on('updateCurrentStateOnClientSide', (set) => {
        console.log('Received updated system data:', set);
        updateUI(set);
    });

    socket.on("isConnectedToMaster", (status) => {
        console.log('Master connection status:', status);
        $('#connectionToMaster-status').text(status ? 'TRUE' : 'FALSE');
    });

    // Handle disconnection
    socket.on('disconnect', () => {
        console.log('Disconnected from the server');
        $('#connectionToMaster-status').text('DISCONNECTED');
    });

    // Toggle buttons (ON/OFF)
    $('.toggledButton').each(function () {
        $(this).on('click', function () {
            const action = $(this).text() === 'ON' ? 'OFF' : 'ON';
            socket.emit("btnAction", { btnName: this.id, action }, (response) => {
                console.log(response);
            });
            $(this).text(action);
        });
    });

    // Handle form submissions
    $("#manualSettingsForm").submit((event) => {
        event.preventDefault();
        const data = $(event.currentTarget).serialize();
        if (manualSettingsFormValidate(data)) {
            $.ajax({
                type: "POST",
                url: "/api/manualSettingsForm",
                data: data,
                success: (response) => console.log("Server response:", response),
                error: (xhr, status, error) => alert(`Error: ${status} - ${error}`)
            });
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
            if(zoneSchedules){
            schedules.push({
                name: `Zone${zoneNumber}`,
                schedules: zoneSchedules
            });
            }
            zoneNumber++;
        });
        console.log(schedules);
        //Prepare data for sending
        const dataToSend = { zones: schedules };
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
        $(".sensors").append(`<p>${sensor.name}: <span>${sensor.value} ${sensor.unit}</span></p>`);
    });
}

const addSchedule = (zoneName, startVal, endVal) => {
    //const zone = $(this).data('zone');
    //zoneName = "zone1";
    //console.log(zoneName);
    const zoneNumber = zoneName.split(" ")[1];
    const scheduleHTML = `
        <div class="schedule">
            <p>
                <label for="zone${zoneNumber}-start">Start Time:</label>
                <input type="time" class="input-field start"  value="${startVal}" name="zone${zoneNumber}Start[]" required>
            </p>
            <p>
                <label for="zone${zoneNumber}-end">End Time:</label>
                <input type="time" class="input-field end" value="${endVal}" name="zone${zoneNumber}End[]" required>
            </p>
        </div>`;
    $(`#${zoneName} .schedules`).append(scheduleHTML);
}

let makeValidTime = (hours, minutes)=>{
    hours=hours.toString();
    minutes=minutes.toString();
    var newHours="";
    var newMinutes="";

    if(hours < 10) {
        newHours+="0";
    }
    if(minutes < 10){
        newMinutes+="0";
    }
    //console.log(hours);
    //console.log(minutes);
    newHours+=hours;
    newMinutes+=minutes;
    console.log(newHours);
    console.log("mew time " + newHours+":"+newMinutes);
    return (newHours+":"+newMinutes);
}
