
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
      text-align: left;
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
      font-size: 16px;
      width: 100px;
      margin-right: 10px;
      border-radius: 4px;
      transition-duration: 0.4s;
    }

    input[type=submit]:hover {
      background-color: #1282A2;
    }

    input, select {
      width: 200px;
      padding: 12px 20px;
      margin: 5px;
      border: 1px solid #ccc;
      border-radius: 4px;
      box-sizing: border-box;
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
    table {
      margin: 0 auto; /* or margin: 0 auto 0 auto */
    }
  </style>
</head>
<body>
  <div class="content">
    <div class="card-grid">
      <div class="card">
        <form action="/get">
        	<table>
            <tr>
                <td>Wifi SSID:</td>
                <td><input value="Unifi" type="text" id="ssid" name="ssid"></td>
            </tr>
            <tr>
                <td>Wifi Password:</td>
                <td><input type="text" id="pass" name="pass"></td>
            </tr>
            <tr>
                <td>MQTT Server:</td>
                <td><input value="192.168.192.26" type="text" id="server" name="server"></td>
            </tr>
            <tr>
                <td>MQTT Port:</td>
                <td><input value="1883" type="number" id="port" name="port"></td>
            </tr>
            <tr>
                <td>Device Name:</td>
                <td><input value="MainDevice" type="text" pattern="[A-Za-z_]+" title="Only letters and '_' are allowed!" id="name" name="name"></td>
            </tr>
            <tr>
                <td>Select Model:</td>
                <td>
                    <select id="SelectModel" name="SelectModel" onchange="modelChanged()">
                        <option selected="selected" value=ShellyPlus-1(PM)>ShellyPlus-1(PM)</option>
                        <option value=ShellyPlus-2PM>ShellyPlus-2PM</option>
                        <option value=ShellyPlus-i4>ShellyPlus-i4</option>
                    </select>
                </td>
            </tr>
            <tr id="rowDeviceMode" style="display:none">
                <td>Device Mode:</td>
                <td>
                    <select id="deviceMode" name="deviceMode">
                        <option selected="selected" value=Light>Light</option>
                        <option value=Cover>Cover</option>
                    </select>
                </td>
            </tr>
            <tr id="rowInput1">
                <td>Input 1:</td>
                <td>
                    <select id="input1" name="input1">
                        <option selected="selected" value=Switch>Switch</option>
                        <option value=Button>Button</option>
                        <option value=Detached>Detached</option>
                    </select>
                </td>
            </tr>
            <tr id="rowInput2" style="display:none">
                <td>Input 2:</td>
                <td>
                    <select id="input2" name="input2">
                        <option selected="selected" value=Switch>Switch</option>
                        <option value=Button>Button</option>
                        <option value=Detached>Detached</option>
                    </select>
                </td>
            </tr>
            <tr>
                <td colspan="2" style="margin: 0 auto"><input type="submit" value="Submit"></td>
            </tr>
            <tr>
                <td colspan="2"></td>
            </tr>
          </table>
        </form>
      </div>
    </div>
  </div>
<script>
	function modelChanged() {
		let value = document.getElementById("SelectModel").value;
        if ( value==="ShellyPlus-1(PM)"){
        	document.getElementById("rowDeviceMode").style.display = "none";
            document.getElementById("rowInput1").style.display = "";
            document.getElementById("rowInput2").style.display = "none";
        }
        else if ( value==="ShellyPlus-2PM"){
        	document.getElementById("rowDeviceMode").style.display = "";
            document.getElementById("rowInput1").style.display = "";
            document.getElementById("rowInput2").style.display = "";
		}
        else if ( value==="ShellyPlus-i4"){
        	document.getElementById("rowDeviceMode").style.display = "none";
            document.getElementById("rowInput1").style.display = "none";
            document.getElementById("rowInput2").style.display = "none";
        }
	}
</script>
</body>
</html>
)rawliteral";
