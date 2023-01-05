
#include <Arduino.h>

const char captive_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <link rel="stylesheet" type="text/css" href="style.css">
  <style>
    html {
      font-family: Arial, Helvetica, sans-serif; 
      display: inline-block; 
      text-align: center;
    }

    p { 
      font-size: 1.4rem;
    }

    body {  
      margin: 0;
    }

    .content { 
      padding: 5%;
    }

    .card-grid { 
      max-width: 800px; 
      margin: 0 auto; 
      display: grid; 
      grid-gap: 2rem; 
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    }

    .card { 
      background-color: white; 
      box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    }

    .card-title { 
      font-size: 1.2rem;
      font-weight: bold;
      color: #034078
    }

    input[type=submit] {
      border: none;
      color: #FEFCFB;
      background-color: #034078;
      padding: 15px 15px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 16px;
      width: 100px;
      margin-right: 10px;
      border-radius: 4px;
      transition-duration: 0.4s;
    }

    input[type=submit]:hover {
      background-color: #1282A2;
    }

    input[type=text], input[type=number], select {
      width: 200px;
      padding: 12px 20px;
      margin: 18px;
      display: inline-block;
      border: 1px solid #ccc;
      border-radius: 4px;
      box-sizing: border-box;
    }

    label {
      font-size: 1.2rem; 
    }
    .value{
      font-size: 1.2rem;
      color: #1282A2;  
    }
    .state {
      font-size: 1.2rem;
      color: #1282A2;
    }
    button {
      border: none;
      color: #FEFCFB;
      padding: 15px 32px;
      text-align: center;
      font-size: 16px;
      width: 100px;
      border-radius: 4px;
      transition-duration: 0.4s;
    }
    .button-on {
      background-color: #034078;
    }
    .button-on:hover {
      background-color: #1282A2;
    }
    .button-off {
      background-color: #858585;
    }
    .button-off:hover {
      background-color: #252524;
    } 
  </style>
</head>
<body>
  <div class="content">
    <div class="card-grid">
      <div class="card">
        <form action="/get">
          <br>
          <label for="ssid">Wifi SSID:</label>
          <br>
          <input value="Unifi" type="text" id="ssid" name="ssid">
          <br>
          
          <label for="pass">Wifi Password:</label>
          <br>
          <input type="text" id="pass" name="pass">
          <br>
          
          <label for="server">MQTT Server:</label>
          <br>
          <input value="192.168.192.26" type="text" id="server" name="server">
          <br>
          
          <label for="port">MQTT Port:</label>
          <br>
          <input value="1883" type="number" id="port" name="port">
          <br>
          
          <label for="name">Device Name: (don't use special characters!)</label>
          <br>
          <input value="master" type="text" id="name" name="name">
          <br>
          
          <label for="mode">Device Mode:</label>
          <br>
          <select id="mode" name="mode">
            <option selected="selected" value=LIGHT>LIGHT</option>
            <option value=COVER>COVER</option>
          </select>
          <br>
          <label for="name">Config</label>
          <br>
          <input value="optional" type="text" id="config" name="config">
          <br>
          <br>
          <input type="submit" value="Submit">
          <br>
        </form>
      </div>
    </div>
  </div>
</body>
</html>
)rawliteral";
