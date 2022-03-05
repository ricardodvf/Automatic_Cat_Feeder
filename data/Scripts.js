// Define our gateway as the address allocated to our application and then declare a variable for the websocket
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

//Initiating the WebSocket and adding the handlers
function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    //websocket = new WebSocket(gateway);
    websocket = new WebSocket(`ws://10.0.0.44/ws`);
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
        document.getElementById('state').innerHTML = state;
        $('#alert_feederON').show();
        $('#myToast').show();
    } else if (event.data == "0") {
        state = "OFF";
        document.getElementById('button_Manual').innerHTML = "TURN ON";
        document.getElementById('state').innerHTML = state;
    } else if (event.data == "AutoON") {
        document.getElementById('button_Automatic').innerHTML = "TURN OFF AUTOMATIC";
        document.getElementById('switch_AutomaticFeeding').checked = true;
    } else if (event.data == "AutoOFF") {
        document.getElementById('button_Automatic').innerHTML = "TURN ON AUTOMATIC";
        document.getElementById('switch_AutomaticFeeding').checked = false;
    } else if (event.data.startsWith("time:")) {
        let commandPosition = event.data.indexOf("time:") + 6;
        document.getElementById('timeLabel').innerHTML = event.data.substring(commandPosition);
    }

}

// Adding a listener to the window when we load it
window.addEventListener('load', onLoad);
// onLoad runs when the page is loaded, we start the websocket and add a listener to the button directing it to the toggle function.
function onLoad(event) {
    $(".alert").hide()
    initWebSocket();
    initButton();
}



//Add the listener to all the components and directs them to the desired function
function initButton() {
    document.getElementById('button_Manual').addEventListener('click', button_Manual_Click);
    document.getElementById('button_Automatic').addEventListener('click', button_Automatic_Feeding);
    document.getElementById('switch_AutomaticFeeding').addEventListener('change', button_Automatic_Feeding);
    document.getElementById('hrsBetweenFeeding').addEventListener('change', changes);
    document.getElementById('secondsToFeed').addEventListener('change', changes);
    document.getElementById('timeFrom').addEventListener('change', changes);
    document.getElementById('timeTo').addEventListener('change', changes);

}

//Functions to send a message to the server.
function button_Manual_Click() {
    websocket.send('manual_toggle');
    $('#myToast').show();
    setTimeout(() => {
        $('#myToast').hide();
    }, 2000);
}

function changes() {
    websocket.send('secondsToFeed: ' + document.getElementById('secondsToFeed').value);
}

function button_Automatic_Feeding() {
    websocket.send('auto_toggle');
    $('#myToast').hide();
}