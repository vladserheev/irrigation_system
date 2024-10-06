$(document).ready(function() {
    // Initially show the Direct Control section
    $('#directControl').show();
    $('.status').not('#directControl').hide();

    // Add click event to navigation buttons
    $('.nav-button').on('click', function() {
        // Remove active class from all buttons and hide all sections
        $('.nav-button').removeClass('active');
        $('.status').hide();

        // Add active class to the clicked button and show the corresponding section
        $(this).addClass('active');
        const target = $(this).data('target');
        $(target).show();
    });
});
$(document).ready(function() {

    // Function to update the display value
    function updateDisplay($element, value) {
        $element.text(value);
    }

    // Attach input event to all range inputs
    $('.range-input').on('input', function() {
        const displayId = $(this).data('display'); // Get the display ID from data attribute
        const displayValue = $(`span[data-display="${displayId}"]`); // Find the corresponding display element
        updateDisplay(displayValue, $(this).val()); // Update the display
    });


});

const buttons = document.querySelectorAll('.toggledButton');

const manualSettingsFormSubmit = (form) => {
    console.log(form);
};

// Add event listener to each button
buttons.forEach(button => {
    button.addEventListener('click', function() {
        // Toggle button text between "ON" and "OFF"
        socket.emit("btnAction", {btnName: button.id, action: button.textContent});
        if (button.textContent === 'ON') {
            button.textContent = 'OFF';
        } else {
            button.textContent = 'ON';
        }
    });
});

const socket = io();
// Listen for connection event
socket.on("connect", () => {
    console.log("Connected to the server!");
    socket.emit("client", true);
    socket.emit('checkIfConnectedToMaster', null, (status) => {
        // This callback handles the server's response
        console.log('Received status from server:', status);  // Log the received status
        document.getElementById('connectionToMaster-status').textContent = status ? 'TRUE' : 'FALSE';
    });
});

// Listen for the current state
socket.on('sendCurrentState', (data) => {
    console.log('Received initial system data:', data);
    updateUI(data);  // Update the UI when the data is first received
});

// Listen for updates to the system's state
socket.on('updateCurrentStateOnClientSide', (set) => {
    console.log('Received updated system data:', set);
    updateUI(set);  // Update the UI when the system state is updated
});

socket.on("isConnectedToMaster", (status) => {
    console.log('Received status from server:', status);  // Log the received status
    document.getElementById('connectionToMaster-status').textContent = status ? 'TRUE' : 'FALSE';
})

// Function to update the UI with the current state of the system
function updateUI(data) {
    // Update pump status
    document.getElementById('pump-status').textContent = data.pump ? 'ON' : 'OFF';

    // Update valves status
    document.getElementById('valve1-status').textContent = data.valve1 ? 'Open' : 'Closed';
    document.getElementById('valve2-status').textContent = data.valve2 ? 'Open' : 'Closed';
    document.getElementById('valve3-status').textContent = data.valve3 ? 'Open' : 'Closed';

    // Update sensor values
    document.getElementById('soilMoisture1').textContent = `${data.soilMoistureSensor1.value} ${data.soilMoistureSensor1.unit}`;
    document.getElementById('soilMoisture2').textContent = `${data.soilMoistureSensor2.value} ${data.soilMoistureSensor2.unit}`;
    document.getElementById('temperature').textContent = `${data.temperatureSensor.value} ${data.temperatureSensor.unit}`;
    document.getElementById('waterLevel').textContent = `${data.waterLevelSensor.value} ${data.waterLevelSensor.unit}`;
    document.getElementById('airHumidity').textContent = `${data.airHumiditySensor.value} ${data.airHumiditySensor.unit}`;
}

$(document).ready(function() {
    $("#manualSettingsForm").submit((event) => {
        console.log("form submitted");
        event.preventDefault(); // Prevent the default form submission
        var data = $(event.currentTarget).serialize(); // Serialize the form data

        $.ajax({
            type: "POST",
            url: "/api/manualSettingsForm", // Ensure the URL is correct
            data: data,
            success: function(response) {
                console.log("Response from server:", response); // Handle success
            },
            error: function(xhr, status, error) {
                console.error("AJAX Error:", status, error); // Handle error
            }
        });
    });
});