// This file contains blocks of HTML and/or Javascript stored in variables.  These are 
// reference mainly from Server.ino.  
// Things related to ramp plan management are in a separate file:
#include "WebRamp.h"


/* Boilerplate for adding variables.  Just change "varname" and add lines between the ().
const char varname[] PROGMEM = R"rawliteral(
)rawliteral";
 */
// Note that in many cases a placeholder between "~" characters is used, for example ~TITLE~.
// This is replaced before sending as defined in the process() function in Server.ino
// That character must not be used for any other purpose.

/* A list of links to other pages. Included this on most or all pages served.  */
const char linkList[] PROGMEM = R"rawliteral(
<div class="wrapper flex fittwowide" style="height:fit-content;">
<b>Any Time</b><br/>
<ul>
<li><a href="/files">List files on SD.</a> </li>
<li><a href="/Tchart.html">Monitor temperatures.</a></li>
<li><a href="/About">About CBASS-32.</a></li>
</ul>
<b>Limit Use During Experiments</b><br/>
<ul>
<li><a href="/RampPlan">Manage ramp plan.</a></li>
<li><a href="/SyncTime">Synchronize CBASS time to device.</a></li>
<li><a href="/ResetRampPlan">Reset ramp plan to example values.</a></li>
<li><a href="/LogManagement">Manage the log file.</a></li>
~UPLOAD_LINK~
<li><a href="/Reboot">Reboot CBASS.</a></li>
</ul>
Items in the lower section can change the experimental plan or delay logging.
They will work during an experimental ramp, but best practice is to use them for 
setup and then after the experiment for obtaining the log file.
</div>
)rawliteral";

const char header[] PROGMEM = R"rawliteral(
<html><head><title>~TITLE~</title></head><body>
)rawliteral";

/* Base (home) page.  Almost too simple to bother with this
 * approach, but it's a test for more complex pages.
 */
const char basePage[] PROGMEM = R"rawliteral(
<html><head><title>~TITLE~</title>
  <link rel="stylesheet" type="text/css" href="/page.css" />
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head><body>
<h2>Welcome to CBASS-32</h2>
Less wiring, more science!
~LINKLIST~
</body></html>
)rawliteral";

/* About page.  A wordier version than the base page with some useful links
 * credits, and whatever comes to mind.
 */
const char aboutPage[] PROGMEM = R"rawliteral(
<html><head><title>~TITLE~</title>
  <link rel="stylesheet" type="text/css" href="/page.css" />
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head><body>
<h2>About CBASS-32</h2>
Less wiring, more science!

<h3>CBASS-32 offers</h3>
<ul>
<li>Exactly the same thermal control algorithm as the original CBASS and CBASS-R, for consistent scientific results.
<li>A web interface for easier management, eliminating the need for frequent physical access to the micro SD card.
<li>A smaller, lighter package.
<li>Faster processing and more memory, thanks to the Arduino Nano ESP32.
<li>The ability to control up to 8 tanks.  Displays and logging auto-adapt to any number from 1 to 8.
<li>A small cost savings, thanks to a box which can be printed on almost any 3D printer.
</ul>
<h3>Manuals and References</h3>
<ul>
<li><a href="https://tinyurl.com/CBASS-32">CBASS-32 Manual</a> Acquiring, assembling, and connecting CBASS-32.
<li><a href="https://github.com/VeloSteve/CBASS-32">CBASS-32 Source Code</a> Temporarily private.
<li><a href="https://aslopubs.onlinelibrary.wiley.com/doi/full/10.1002/lom3.10555">The Coral Bleaching Automated Stress System (CBASS): A low-cost, portable system for standardized empirical assessments of coral thermal limits</a>  The main Evensen et al. paper defining CBASS.
<li><a href="https://onlinelibrary.wiley.com/doi/10.1111/gcb.15148">Standardized short-term acute heat stress assays resolve historical differences in coral thermotolerance across microhabitat reef sites</a> An earlier paper by Voolstra, et al. establishing the applicability of CBASS assays.
<li><a href="https://plotly.com/javascript/">Plotly,</a> the library used to display temperatures and ramp plans.
<li><a href="https://store-usa.arduino.cc/products/nano-esp32?selectedStore=us">Arduino purchase,</a> <a href="https://docs.arduino.cc/hardware/nano-esp32/">documentation,</a> and <a href="https://docs.arduino.cc/tutorials/nano-esp32/cheat-sheet/">cheat sheet.</a>
</ul>
<img src="/32Board.png" border="0" width="360" height="512" alt="CBASS-R v0.2 Board" title="Board Version 0.2"/></div>

~LINKLIST~
</body></html>
)rawliteral";

#ifdef ALLOW_UPLOADS
const char uploadHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <link rel="stylesheet" type="text/css" href="page.css" />  
  <title>File Upload</title>
  <script>
    function updateNewName() {
      // newdir is a typed new directory name
      // dirChoices are in the dropdown
      // fileToUpload is from the chooser
      console.log("Directory option changed");
      let chosenDir = document.getElementById('dirChoices').value;
      let nnn = document.getElementById("newdir");
      let fileChooser = document.getElementById("fileToUpload");
      let button = document.getElementById("upload");
      var upName;
      // Check that an item exists, otherwise name is a property of a null object - a problem.
      if (fileChooser.files.item(0)) {
        upName = fileChooser.files.item(0).name;
      }

      if (chosenDir == "--new--") {
        nnn.disabled = false;
        // If new directory is enabled, we should not submit unless the blank is filled and a file has been chosen.
        console.log("New dir name:");
        console.log(nnn.value);
        console.log("Upload file name:");
        var upName;
        if (upName) {
          console.log(upName);
        } else {
          console.log("No file selected yet.");
        }
        if (nnn.value && upName) {
          button.disabled = false;
        } else {
          button.disabled = true;
        }
      } else {
        nnn.disabled = true;
        if (upName) button.disabled = false;
      }
    }
  </script>
</head>
<body>
  <div class="container">
  <div class="wrapper flex fittwowide">
  <h1>Upload a File</h1>
  <form action="/Upload" method="POST" enctype="multipart/form-data">
    ~DIRECTORY_CHOICE~
    <input type="file" id="fileToUpload" name="fileToUpload" onchange=updateNewName() required>
    <br><br>
    <input type="submit" id="upload" value="Upload" disabled>
  </form>
  </div>
  ~LINKLIST~
  </div>
</body>
</html>
)rawliteral";
#endif

const char dirListHTML[] PROGMEM = R"rawliteral(
  <html><head><title>~TITLE~</title>
  <link rel="stylesheet" type="text/css" href="page.css" />  
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body><div class="container">
  ~DIRLIST~
  ~LINKLIST~
  </div></body></html>
)rawliteral";

const char uploadSuccess[] PROGMEM = R"rawliteral(
  <html><head><title>~TITLE~</title>
  <link rel="stylesheet" type="text/css" href="page.css" />  
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body><div class="container">
  <div class="wrapper flex fittwowide">Upload successful!</div>
  ~LINKLIST~
  </div></body></html>
)rawliteral";

const char rollLogNow[] PROGMEM = R"rawliteral(
    ~ROLLLOG~
)rawliteral";

const char rebootHTML[] PROGMEM = R"rawliteral(
    <html><head><title>~TITLE~
  </title>
  <link rel="stylesheet" type="text/css" href="page.css" />  
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <div class="container"><div class="wrapper flex fittwowide">
    <h1>Rebooting CBASS-32</h1>
    At ~DATETIME~
    <br/>
    The links below should be available in about 12 seconds, and full operation in about 18 seconds.
  </div>
    ~LINKLIST~
  </div>
  </body></html>
)rawliteral";

/* A simple ramp for use when there is nothing usable on the SD card. */
const char defaultINI[] PROGMEM = R"rawliteral(
// A typical ramp preceeded by some tests
// Start time (1 PM in this example)
START 13:00
// LINEAR is the default, so this is just a reminder.
INTERP LINEAR
// Times in column 1 are relative to START.
// The temperatures are targets for each tank in Celsius.
0:00	30	30	30	30
3:00	30	30	33	33
6:00	30	30	33	33
7:00	30	30	30	30
)rawliteral";

const char logHTML2[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
	<title>~TITLE~</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" type="text/css" href="page.css" />
  <style>
    button {
      background: none!important;
      border: none;
      padding: 0!important;
      /*optional*/
      font-family: arial, sans-serif;
      /*input has OS specific font-family*/
      color: #00E;
      text-decoration: underline;
      cursor: pointer;
    }
  </style>
</head>
<body>
<div class="container">
<div id="message" style="color:red;">~RESET_WARNING~</div>
<div class="wrapper flex fittwowide">
<p>The file LOG.txt is an important output of CBASS experiments. 
 It gives the most direct confirmation of the actual temperatures in the 
 test areas, though there may alternate monitoring methods as well.
 Because of this it must be <b>handled with care.</b></p>


<p>You must enter the "Magic Word".  Note that it is not a secure password<br>

<label >Magic Word:
<input type="text" id="magicWord" onchange="addMagic()" name="magicWord"></label><br/>

<ol>
 <li><a id="dl" href="/LogDownload" download>Download the current log file.</a> without changing the file.</li>
 <li><form><button type="submit" id="roll" href="/LogRoll">Archive the log and start a new one.</button></form></li>
</ol>
</div>
<script>
  function addMagic() {
    console.log("adding magic");
    let url = new URL(window.location.origin + "/LogDownload");
    let vvv = document.getElementById('magicWord').value;
    url.searchParams.set("magicWord", vvv);
    console.log(url.href);
    let lll = document.getElementById("dl");
    lll.href = url.href;
  }

  const form = document.querySelector('form');
  form.addEventListener('submit', async (e) => {
    console.log("roll link clicked");
    e.preventDefault();

    //for (const pair of formData.entries()) {
    //  console.log(pair);
    //}
 
    mmm = document.getElementById('magicWord').value;
    const searchParams = new URLSearchParams();
    searchParams.append("magicWord", mmm)
    let response = await fetch ("/LogRoll/?" + searchParams.toString());
    const msg = await response.text();
    console.log("Message is " + msg);
    console.log("response is");
    console.log(response);
    console.log("with status");
    console.log(response.status);

    const messageArea = document.getElementById('message');
    if (response.status == 200) {
      messageArea.innerHTML = "<b>Log roll action successful.</b> to " + msg; 
      messageArea.style.color = 'blue';
    } else {
      messageArea.innerHTML = "ERROR " + response.status + " " + msg;
      messageArea.style.color = 'red';
    }

  });


</script>
~LINKLIST~
</div></body></html>
)rawliteral";



// A page for synchronizing CBASS-32 time to the current device.
const char syncTime[] PROGMEM = R"rawliteral(
<html>
<head>
	<title>~TITLE~</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" type="text/css" href="page.css" />  
</head>
<body>
<div class="container">
<div id="message" style="color:red;">~RESET_WARNING~</div>
<div class="wrapper flex fittwowide">
<p>This page allows you to synchronize CBASS time and date to the current device.
<p>CBASS does not know about time zones or "Daylight Saving".  You will
need to reset the clock from here if you want to match those changes.

<p>Most recent CBASS time: ~DATETIME~

<p>To change time you must enter the "Magic Word".  It is not a secure password.<br>
<form>
<label for=\"MagicWord\">Magic Word:</label>
<input type="text" id="magicWord" name="magicWord">
<input name="sync" type="hidden" value="true">

<input type="submit" value="Sync Clock Now"><br><br>
</form>
</div>
<script>
  const form = document.querySelector('form');
  form.addEventListener('submit', async (e) => {
    console.log("sync listener called");
    e.preventDefault();
    const formData = new FormData(form);
    // Append the current time from the user's device.
    const now = new Date();
    // "en-GB" because I want day before month.
    const toSend = now.toLocaleDateString('en-GB') + ' ' +  now.toTimeString();
    // The time has GMT followed by time zone information we will ignore.
    const trimmed = toSend.substring(0, toSend.indexOf('GMT'));
    formData.append('datetime', trimmed);
    for (const pair of formData.entries()) {
      console.log(pair);
    }
 
    const searchParams = new URLSearchParams(formData);
    const response = await fetch ("/updateDateTime/?" + searchParams.toString());
    const msg = await response.text();
    console.log("Message is " + msg);
    console.log("response is");
    console.log(response);
    console.log("with status");
    console.log(response.status);

    const messageArea = document.getElementById('message');
    if (response.status == 200) {
      messageArea.innerHTML = "<b>Time and date updated successfully</b> to " + msg; 
      messageArea.style.color = 'blue';
    } else {
      messageArea.innerHTML = "ERROR " + response.status + " " + msg;
      messageArea.style.color = 'red';
    }

  });


</script>
~LINKLIST~
</div></body></html>
)rawliteral";


/* 
 * The temperature monitoring page HTML and Javascript.
 */
const char plotlyHTML[] PROGMEM = R"rawliteral(<html>
<head>
	<title>~TITLE~</title>
	<script src="plotly.js" charset="utf-8"></script>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" type="text/css" href="/page.css" />
</head>
<body>
<div class="container">
		
<div id="historyPlot"  class="wrapper flex fittwowide" ></div>
<div id="statusTable"  class="wrapper flex fittwowide" style="height:150px;">
~TABLE_NT~
</div>
<!-- <div id="bbb"  class="wrapper flex fittwowide"  style="height:30px;">Nothing yet.</div> -->

<script>
	TESTER = document.getElementById('historyPlot');

  // Colors from Paul Tol, which should be relatively color-blind friendly
  //  This is his "vibrant" scheme, with black added because we want 8 colors.
  //  His suggested order is:  orange  blue  cyan  magenta  red  teal  grey
  const lineColors = [
    '#EE7733',
    '#0077BB',
    '#33BBEE',
    '#EE3377',
    '#CC3311',
    '#009988',
    '#BBBBBB',
    '#000000'
   ];


  var latest = -1; // Latest timestamp already plotted.
  var oldLatest = -2;  // If they have the same one twice the call will be skipped.
  var pointsReceived = 0;
  var data = [];
  var trace;
  // NT temperatures
  for (let i=0; i < ~NT~; i++) {
    trace = {x: [],
      y: [],
      mode: 'lines', 
      name: 'Tank '+(i+1),
      line: {color: lineColors[i % 8], width: 3}
    };
    data.push(trace);
  }
  // NT targets
  for (let i=0; i < ~NT~; i++) {
    trace = {x: [],
      y: [],
      mode: 'lines',
      name: 'Target '+(i+1),
      line: {color: lineColors[i % 8], width: 1}
    }
    data.push(trace);
  }
	tHistory = Plotly.newPlot( TESTER, data, {
	margin: { t: 50 },
  title: 'Temperature History',
  xaxis: {title: 'Time of Day'},
  yaxis: {title: 'T, '+ String.fromCodePoint(176) + 'C'},  // Just pasting ° didn't work 176 gives the symbol and 8451 gives it with the C.
  responsive: true  // Resize when window resizes, so CSS can fit on different devices.
   }, {modeBarButtonsToRemove: ['resetScale2d']} );

  // Get the first actual data points right away, later it will be repeated as defined by setInterval.
  // Now we may be getting batches of points.  Large batches mean we are catching up after starting
  // the graph well after CBASS started.  Do that a little more aggressively than once we are caught up.

  // Get data points.
  // The default maximum batch size on the CBASS-32 side is 1000 points, but we'll only be getting 1 to 6 points
  // once the graph catches up to real time.
  // Note that an early version with setInterval was unreliable.  setTimeout is more suitable for sequential
  // actions of unknown duration.
  console.log("Starting point collection.");

  pointsReceived = 10000;
  let fetchDelay = 1000;  // Start getting points every second for fast startup.
  function collectData() {
    getPoints(TESTER)
      .then(() => {
        if (true) {
          // Slow down to every 5 seconds once we catch up.
          if (pointsReceived < 100 && pointsReceived > 0) fetchDelay = 5000;
          setTimeout(collectData, fetchDelay);
        }
      })
      .catch(e => console.log(e));
  }
  collectData();



  // must be async to use await.
  async function getPoints(TESTER) {
    var debug = 0;
    if (latest == oldLatest) return;
    let jjj;
    if (debug) console.log("last rec = " + pointsReceived + " latest = " + latest + " fetchDelay = " + fetchDelay);
    const res = await fetch("http://~IP~/runT?oldest=" + (latest+1));
    // console.log('Response status: ' + res.status);
    if (!res.ok) {
      throw new Error(`HTTP error: ${res.status}`);
    }
    jjj = await res.json();

    // Input looks like
    //  {"NT":4,"points":{"24307":{"datetime":"2024-04-12T08:38:28","target":[24.00,24.00,24.00,24.00],"actual":[22.13,22.56,22.06,22.44]},
    //                    "29312":{"datetime":"2024-04-12T08:38:33","target":[24.00,24.00,24.00,24.00],"actual":[22.25,22.56,22.13,22.44]}}
    //  }');

    pointsReceived = Object.keys(jjj.points).length;
    if (debug) console.log("Received " + pointsReceived + " points.  Latest was " + latest);
    if (pointsReceived == 0) return;
    oldLatest = latest;

    //if (~NT~ != jjj.NT) {
    //  document.getElementById('bbb').innerText="ERROR: NT in data (" + jjj.NT + ") != NT of this page (" + ~NT~ + ")";
    //} 

    // Note that we have two categories of things to do. 
    // 1) update the traces with all points received and update "latest"
    // 2) update text on the page using only the last (most recent) point.

    // === Update Traces ===
    var data = [];
    // Note that syntax like times = [][] is illegal in Javascript.  One level at a time!
    // This is awkward.  We want lists of points per tank, but the data comes
    // in with all the tank values for a time points.  We need to build the
    // empty 2D array first, and then fill it.
    // Fancy array-initialization code from https://stackoverflow.com/questions/3689903/how-to-create-a-2d-array-of-zeroes-in-javascript
    let times = Array(2 * jjj.NT).fill().map(() => Array(pointsReceived));
    let temps = Array(2 * jjj.NT).fill().map(() => Array(pointsReceived));  // Double length due to targets.
    let n = 0;
    for (var key in jjj.points) {
      latest = parseInt(key);
      const pt = jjj.points[key];
      for (let i=0; i < jjj.NT; i++) {
        //times[i][n] = latest
        times[i][n] = pt.datetime;
        times[i + jjj.NT][n] = pt.datetime;
        temps[i][n] = parseFloat(pt.actual[i]);
        temps[i + jjj.NT][n] = parseFloat(pt.target[i]); // targets after actual temps
      }
      n++;
    }
    data = {x:times, y:temps};

    if (debug) console.log("Extending traces.  New latest is ", latest);

    //console.log(data);
 
    // For a new plot with 2 lines we send x1 y1 x2 y2
    // but to extend it's                  x2 x2 y1 y2!
    // Each single item below could be multiple, if updating in batches.
    /*
    data = {x: [[newTime],[newTime],[newTime],[newTime]],
	          y: [[newTemp[0]],[newTemp[1]],[newTemp[2]],[newTemp[3]]]
           };
     */
    // Tried this funny dot notation - several variants never were accepted by plotly.
    //    Plotly.extendTraces(TESTER, data, [[...Array(~NT~).keys()]], 40);
    // NT * 2 for NT targets and NT actual temps
    var listNT = new Array(~NT~ * 2);  // note that = [NT] would be an array of length 1 and value NT
    for (let i=0; i < ~NT~ * 2; i++) {
      listNT[i] = i;
    }

    // Last number is a point limit.  This should really be coordinated with the 
    // number of points being generated, but for now assume a typical target of
    // being able to plot one point per 5 seconds for 6 hours.
    Plotly.extendTraces(TESTER, data, listNT, 6*3600/5);

    // === Update text information ===
    pt = jjj.points[latest];
    //document.getElementById('bbb').innerText=pt.actual;

    let delta = 0.0;
    for (let i=1; i <= ~NT~; i++) {
      document.getElementById('temp' + i).innerHTML=pt.actual[i-1]; //jjj.tempList[i-1];
      delta = parseFloat(pt.actual[i-1]) - parseFloat(pt.target[i-1]);
      if (delta > 0.49) {
        document.getElementById('temp' + i).style.backgroundColor="#FF8888";
      } else if (delta < -0.49) {
        document.getElementById('temp' + i).style.backgroundColor="AAAAFF";
      } else {
        document.getElementById('temp' + i).style.backgroundColor="#f8f8f8";
      }
      document.getElementById('set' + i).innerHTML=pt.target[i-1];
    }

    const now = new Date();
    document.getElementById('cbassTime').innerText=`CBASS time: ${pt.datetime}`
    document.getElementById('updateTime').innerText=`Last update: ${now.getHours()}:${String(now.getMinutes()).padStart(2, '0')}`

  }

 
</script>
~LINKLIST~
</div>
</body></html>
)rawliteral";

