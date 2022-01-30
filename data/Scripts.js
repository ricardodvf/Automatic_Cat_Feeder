// Define our gateway as the address allocated to our application and then declare a variable for the websocket
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

//Initiating the WebSocket and adding the handlers
function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

//Message to the console when websocket opens
function onOpen(event) {
    console.log('Connection opened');
}

//Message to console when websocket closes
function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

// Meat and potatoes... handle received messages from the websocket
function onMessage(event) {
    var state;
    console.log(event.data);
    if (event.data == "1") {
        state = "ON";
        document.getElementById('button_Manual').innerHTML = "TURN OFF";
        document.getElementById('state').innerHTML = "Cat Feeder Status: " + state;
        $('#alert_feederON').show();
    } else if (event.data == "0") {
        state = "OFF";
        document.getElementById('button_Manual').innerHTML = "TURN ON";
        document.getElementById('state').innerHTML = "Cat Feeder Status: " + state;
    } else if (event.data == "AutoON") {
        document.getElementById('button_Automatic').innerHTML = "TURN OFF AUTOMATIC";
        document.getElementById('switch_AutomaticFeeding').checked = true;
    } else if (event.data == "AutoOFF") {
        document.getElementById('button_Automatic').innerHTML = "TURN ON AUTOMATIC";
        document.getElementById('switch_AutomaticFeeding').checked = false;
    }

}

// Adding a listener to the window when we load it
window.addEventListener('load', onLoad);
// onLoad runs when the page is loaded, we start the websocket and add a listener to the button directing it to the toggle function.
function onLoad(event) {
    $(".alert").hide()
    initWebSocket();
    initButton();
    $(function() {
        $("#includedContent").load("cat_swing.html");
    });
}

//Add the listener to all the components and directs them to the desired function
function initButton() {
    document.getElementById('button_Manual').addEventListener('click', feedCat);
    document.getElementById('button_Automatic').addEventListener('click', startAutomaticFeeding);
    document.getElementById('switch_AutomaticFeeding').addEventListener('change', startAutomaticFeeding);
}

//Functions to send a message to the server.
function feedCat() {
    websocket.send('manual_toggle');
}

function startAutomaticFeeding() {
    websocket.send('auto_toggle');
}