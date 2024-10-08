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

    // Update display values for range inputs
    $('.range-input').on('input', function () {
        const displayId = $(this).data('display');
        const displayValue = $(`span[data-display="${displayId}"]`);
        displayValue.text($(this).val());
    });

    // Socket connection setup
    const socket = io();
    socket.on("connect", () => {
        console.log("Connected to the server!");
        socket.emit("client", true); // Notify server of client connection
        socket.emit('checkIfConnectedToMaster', null, (status) => {
            console.log('Master connection status:', status);
            $('#connectionToMaster-status').text(status ? 'TRUE' : 'FALSE');
        });
    });

    // Listen for system state updates
    socket.on('sendCurrentState', (data) => {
        console.log('Received initial system data:', data);
        updateUI(data);
    });

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
            socket.emit("btnAction", { btnName: this.id, action });
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
        const data = $(event.currentTarget).serialize();
        if (timeSettingsFormValidate(data)) {
            $.ajax({
                type: "POST",
                url: "/api/timeSettingsForm",
                data: data,
                success: (response) => console.log("Server response:", response),
                error: (xhr, status, error) => alert(`Error: ${status} - ${error}`)
            });
        }
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
    console.log($('#Valve_3'));
    // Update sensors
    sensors.forEach((sensor) => {
        $(".sensors").append(`<p>${sensor.name}: <span>${sensor.value} ${sensor.unit}</span></p>`);
    });
}

