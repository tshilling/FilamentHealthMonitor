const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
    <head>
        <meta charset="utf-8">
        <meta http-equiv="X-UA-Compatible" content="IE=edge">
        <title></title>
        <meta name="RFX Filament Monitor" content="">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <link rel="stylesheet" href="">
        
        <link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">
        <style>
            .colorSwatch{
                background-color: red;   
            }
        </style>
    
    </head>
    <script>
        let c = 0
        setInterval(()=>{
            updateData();
        },1000)

        function updateData() {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    
                    let json = JSON.parse(this.responseText)
                    document.getElementById("rfxWeight").innerText = json.WEIGHT;
                    document.getElementById("rfxTemp").innerText = json.TEMP;
                    document.getElementById("rfxHumidity").innerText = json.HUMIDITY;
                    document.getElementById("rfxColor").style.backgroundColor = "rgb("+json.RED+","+json.GREEN+","+json.BLUE+")"
                    //document.getElementById("rfxColor").innerText = json.RED + "\n"+json.GREEN + "\n"+json.BLUE+ "\n"+json.CLEAR
                    console.log(json);
                //    document.getElementById("demo").innerHTML = this.responseText;
                }
            };
            xhttp.open("GET", "getData", true);
            xhttp.send();
        }
    </script>

    <body class="w3-container" style="padding:10px">
        <div class="w3-card">
                <table class="w3-table-all w3-card-4 w3-center w3-border">
                    <tr class="w3-center w3-blue-grey w3-border"> 
                        <th class="w3-center w3-border" width="10%">Color</th>
                        <th class="w3-center w3-border" width="20%">Weight (g)</th>
                        <th class="w3-center w3-border" width="20%">Temp (C)</th>
                        <th class="w3-center w3-border" width="20%">Humidity (%)</th>
                        <th class="w3-center w3-border" width="30%">IP</th>
                    </tr>
                    <tr>
                        <td id="rfxColor" class="w3-center w3-border w3-dropdown-hover">
                            -
                            <div class="w3-dropdown-content w3-bar-block w3-card-4">
                                <div class="w3-bar-item">
                                    <form action="calWhite" target="votar">
                                            <input class="w3-button w3-white w3-border w3-card" type="submit" style="width:100%" value="Cal White">
                                    </form>
                                </div>
                                <div class="w3-bar-item">
                                    <form action="calBlack" target="votar">
                                            <input class="w3-button w3-black w3-border w3-card" type="submit" style="width:100%" value="Cal Black">
                                    </form>
                                </div>
                            </div>

                        </td>
                        <td  class="w3-center w3-border w3-dropdown-hover">
                            <div id="rfxWeight">1000</div>
                            <div class="w3-dropdown-content w3-bar-block w3-card-4">
                        
                                <div class="w3-bar-item" style="width:100%">
                                    <form action="tare" target="votar">
                                        <input class="w3-button w3-teal w3-border w3-card" type="submit" style="width:100%" value="tare">
                                    </form>
                                </div>
                                <div class="w3-bar-item">
                                    <form action="scale" target="votar">
                                        <input type="number" name="mass" min="0" max="10000" step="1" value="0" style="width:30%">
                                        <input class="w3-button w3-teal w3-border w3-card" type="submit" style="width:65%" value="Weight">
                                    </form>
                                </div>
                                <div class="w3-bar-item">
                                    <form action="offset" target="votar">
                                        <input type="number" name="mass" min="-1000" max="1000" step="1" value="225" style="width:30%">
                                        <input class="w3-button w3-teal w3-border w3-card" type="submit" style="width:65%" value="Offset">
                                    </form>
                                </div>

                            </div>

                        </td>
                        <td class="w3-center w3-border" id=rfxTemp></td>
                        <td class="w3-center w3-border" id=rfxHumidity></td>
                        <td class="w3-center w3-border" id=rfxIP>192.168.86.34</td>
                    </tr>
                </table>
        </div>    
        <iframe name="votar" style="display:none"></iframe>
    </body>
</html>
)=====";