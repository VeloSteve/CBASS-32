/* Boilerplate for adding variables.  Just change "varname" and add lines between the ().
const char varname[] PROGMEM = R"rawliteral(
)rawliteral";
 */
// Note that in many cases a placeholder in backticks is used, for example ~TITLE~.
// This is replaced before sending as define in the process() function in Server.ino

/* A list of links to other pages. Included this on most or all pages served.  */
const char linkList[] PROGMEM = R"rawliteral(
<div class="wrapper">
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
<li>A small cost savings, thank to a box which can be printed on almost any 3D printer.

<li><a href="https://aslopubs.onlinelibrary.wiley.com/doi/full/10.1002/lom3.10555">The Coral Bleaching Automated Stress System (CBASS): A low-cost, portable system for standardized empirical assessments of coral thermal limits</a>  The main Evensen et al. paper defining CBASS.
<li><a href="https://onlinelibrary.wiley.com/doi/10.1111/gcb.15148">Standardized short-term acute heat stress assays resolve historical differences in coral thermotolerance across microhabitat reef sites</a> An earlier paper by Voolstra, et al. establishing the applicability of CBASS assays.
</ul>
<h3>Manuals and Such</h3>
<ul>
<li><a href="https://tinyurl.com/CBASS-32">Acquiring, assembling, and connecting CBASS-32
<li>
</ul>
<img src="/32Board.png" border="0" width="720" height="1024" alt="CBASS-R v0.2 Board" title="Board Version 0.2"/></div>

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
  <h1>Upload a File</h1>
  <form action="/Upload" method="POST" enctype="multipart/form-data">
    ~DIRECTORY_CHOICE~
    <input type="file" id="fileToUpload" name="fileToUpload" onchange=updateNewName() required>
    <br><br>
    <input type="submit" id="upload" value="Upload" disabled>
  </form>
  ~LINKLIST~
</body>
</html>
)rawliteral";
#endif

const char dirListHTML[] PROGMEM = R"rawliteral(
  <html><head><title>~TITLE~</title>
  <link rel="stylesheet" type="text/css" href="page.css" />  
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  ~DIRLIST~
  ~LINKLIST~
  </body></html>
)rawliteral";

/*
const char rollLogNow[] PROGMEM = R"rawliteral(
    <html><head><title>~TITLE~
  </title></head><body>
    ~ROLLLOG~
    ~LINKLIST~</body></html>
)rawliteral";
 */
const char rollLogNow[] PROGMEM = R"rawliteral(
    ~ROLLLOG~
)rawliteral";

const char rebootHTML[] PROGMEM = R"rawliteral(
    <html><head><title>~TITLE~
  </title></head><body><h1>  
    Rebooting CBASS-32  now!<h1/>
    ~LINKLIST~
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

/** DELETE ME
const char logHTML[] PROGMEM = R"rawliteral(
<html>
<head>
	<title>~TITLE~</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" type="text/css" href="/page.css" />
</head>
<body>
<div style="color:red;">~RESET_WARNING~</div>
<p>The file LOG.txt is an important output of CBASS experiments. 
 It gives the most direct confirmation of the actual temperatures in the 
 test areas, though there may alternate monitoring methods as well.
 Because of this it must be <b>handled with care.</b></p>

 <p>The actions are supported here.</p>
<ol>
 <li><a href="/LogDownload" download>Download the current log file.</a> without changing the file.</li>
 <li><a href="/LogRoll">Archive the log and start a new one.</a> This should normally be done only between experiments.</li>
</ol>
Note that either of these actions may cause a brief interruption of logging, so even the download option should be
done with discretion.
~LINKLIST~
</body></html>
)rawliteral";
 */


// A page for synchronizing CBASS-32 time to the current device.
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
<div id="message" style="color:red;">~RESET_WARNING~</div>
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
</body></html>
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
<div id="message" style="color:red;">~RESET_WARNING~</div>
<p>This page allows you to synchronize CBASS time and date to the current device.
<p>This is meant for initial setup or when changing time zones.  If CBASS
forgets the time when powered off and on you should replace the battery.
<p>CBASS does not know about "Daylight Saving" and other time shifts.  You will
need to reset the clock from here if you want to match those changes.

<p>Most recent CBASS time: ~DATETIME~

<p>To change time you must enter the "Magic Word".  Note that it is not a secure password<br>
<form>
<label for=\"MagicWord\">Magic Word:</label>
<input type="text" id="magicWord" name="magicWord"><br/>
<input name="sync" type="hidden" value="true">

<input type="submit" value="Sync Clock Now"><br><br>
</form>
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
</body></html>
)rawliteral";




/* A page for starting a reset of the ramp plan.  Normally only 
 * used when an existing file is lost or a clean start is desired. 
 */
const char iniResetPage[] PROGMEM = R"rawliteral(
<html>
<head>
	<title>~TITLE~</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" type="text/css" href="/page.css" />
</head>
<body>
<div style="color:red;">~ERROR_MSG~</div>
<div class="container">
<div id="resetDiv"  class="wrapper fittwowide" >
Normally you will make changes to an existing Settings.ini file by using the <a href="/RampPlan">Manage ramp plan</a> page.

<p>This page is for use in case the Settings.ini file is lost or you want a clean start.
You will then have a default ramp plan which you will almost certainly need to modify before
running experiments.</p>

Confirm that you want to do this by entering the "Magic Word".  Note that it is not a secure password<br>
<form>
<label for=\"MagicWord\">Magic Word:</label>
<input type="text" id="magicWord" name="magicWord"><br/>
<input name="reset" type="hidden" value="true">
<br>
<input type="submit" value="Reset Settings.ini"><br><br>
</form>
</div>
~LINKLIST~
</div></body></html>
)rawliteral";

/* Javascript for collecting data from a ramp plan table.  This is to include.
 * - as table cells are changed confirm that value are valid, if not mark them and disable the send button
 * - allow table rows to be inserted or deleted (keep at least 2)
 * - collect the data and send to the server
 * - notify the user on success or failure
 */
const char tableScript[] PROGMEM = R"rawliteral(
function submitRampData() {
  const table = document.getElementById('rampTable');
  const toSend = [];

  // Loop through rows and cells to collect data
  for (let i = 1; i < table.rows.length; i++) {  // Start from 1 to skip header row
    const rowData = {
      time: table.rows[i].cells[0].innerText.trim(),
      tempList: Array(~NT~).fill(0)
      //temp1: table.rows[i].cells[1].innerText.trim(),
      //temp2: table.rows[i].cells[2].innerText.trim(),
      //temp3: table.rows[i].cells[3].innerText.trim(),
      //temp4: table.rows[i].cells[4].innerText.trim()
    };
    for (let ii=0; ii < ~NT~; ii++) {
      rowData.tempList[ii] = table.rows[i].cells[ii+1].innerText.trim()
    }
    toSend.push(rowData);
  }

  // now append data not in the table 
  const timeEntry = document.getElementById('StartTime');
  const timeData = { startTime: timeEntry.value.trim() };
  toSend.push(timeData);
  const magicEntry = document.getElementById('MagicWord');
  const magicData = { magicWord: magicEntry.value.trim() };
  toSend.push(magicData);

  console.log("toSend variable:");
  console.log(toSend);
  // Convert toSend data to JSON
  const jsonData = JSON.stringify(toSend);
  console.log("--now the JSON--");
  console.log(jsonData);

  // Send data to server (For simplicity, using fetch to send a POST request to a mock endpoint)
  fetch('/updateRampPlan', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: jsonData
  })
  .then(response => response.json())
  .then(data => {  // data is the parsed response from response.json()!
    console.log('Server response:', data);

    // CHECK FOR error.  message if found, otherwise assume success.
    if (data.msg) {
      console.log("extracted message: ", data.msg);
      alert('Failed: ' + data.msg);
    } else {
      alert('Data saved successfully!');
    }
  })
  .catch(error => {
    console.error('Error:', error);
    //alert('Failed to save data!');
    alert(data);
  });
}

// Remove a row from the table
function removeRow(image) {
  const row = image.parentNode.parentNode;
  row.parentNode.removeChild(row);
}

// Add a new row to the table
function addRow(icon) {
  let newNumber = icon.parentNode.parentNode.rowIndex + 1;
  const table = document.getElementById('rampTable').getElementsByTagName('tbody')[0];
  const newRow = table.insertRow(newNumber);

  // The time
  const cell1 = newRow.insertCell(0);
  cell1.contentEditable = "true"; cell1.className = "time";
  cell1.style.backgroundColor="#ff4444";
  cell1.addEventListener('blur', function() { validateCellTime(cell1); });

   // NT temperatures
  // Is "const" legal here?  Seems wrong.
  for (let i=1; i<=~NT~; i++) {
    const cellTemp = newRow.insertCell(i);
    cellTemp.contentEditable = "true"; cellTemp.className = "temperature";
    cellTemp.style.backgroundColor="#ff4444";
    cellTemp.addEventListener('blur', function() { validateCellTemp(cellTemp); });
  }

  const cellPlus = newRow.insertCell(~NT~ + 1);
  cellPlus.innerHTML = '<img width="24" src="/plus_circle.png" onclick="addRow(this)">';
  const cellTrash = newRow.insertCell(~NT~ + 2);
  cellTrash.innerHTML = '<img width="24" src="/trash.png"  onclick="removeRow(this)">';

  // The new row is blank.  Disable until filled.
  document.getElementById('sendbutton').disabled = true;

}

function hasMagic() {
    mmm = document.getElementById('MagicWord').value.trim()
    return mmm.length > 0;
}
// Check whether the send button should be enabled or not.
function changedMagic() {
    document.getElementById('sendbutton').disabled = (!hasMagic() || !cellsOkay());
}

// Validate each time cell after editing (the first cell in a row)
function validateCellTime(cellElement) {
  if (validateTime(cellElement.innerText.trim())) {
    cellElement.classList.remove('error');
    cellElement.style.backgroundColor="#ffffff"
  } else {
    cellElement.style.backgroundColor="#ff4444"
    cellElement.classList.add('error');
  }

  // Can we enable the send button?
  if (cellElement.classList.contains('error')) {
    // This alone is enough to disable the send button.
    document.getElementById('sendbutton').disabled = true;
  } else {
    // If this one is good and the button is disabled we need to check all the others
    // to see if it can now be enabled.
    if (document.getElementById('sendbutton').disabled) {
      // Iterate!
      if (cellsOkay() && hasMagic()) {
        document.getElementById('sendbutton').disabled = false;
      }
    }
    updatePlot();  // Maybe only if things are okay, but we don't care about the button.

  }
}

/* Take a single value and determine whether it is a valid
 * ramp temperature. This does not modify anything.
 */
function validateTemperature(val) {
  // console.log("Validating a single temperature value >" + val + "<");
  if (!val) {
    // No value
    console.log("no value");
    return false;
  } else if (!Number.isInteger(+val) && !Number.isFinite(+val)) { // + signs required so it make it a number and fails on extra characters
    console.log("not an integer or finite real");

    // Not a number
    return false;
  } else if (val < 5 || val > 50) {
    console.log("too extreme <-------------------------------------");

    // Not a reasonable temperature
    return false;
  }
  //console.log("Value checked as true.");
  return true;
}

/*
 * Validate the temperature of a given cell and toggle its background color
 * as needed. 
 * Then if the cell is okay and the send button is disabled check all cells
 * to see whether we can enable the button.
 * Be careful that this isn't calling recursively.
 */
function validateCellTemp(cellElement) {
  if (validateTemperature(cellElement.innerText.trim())) {
    cellElement.classList.remove('error');
    cellElement.style.backgroundColor="";  // Clear back to default
  } else {
    cellElement.classList.add('error');
    cellElement.style.backgroundColor="#ff4444";
  }
  // Can we enable the send button?
  if (cellElement.classList.contains('error')) {
    // This alone is enough to disable the send button.
    document.getElementById('sendbutton').disabled = true;
  } else {
    // If this one is good and the button is disabled we need to check all the others
    // to see if it can now be enabled.
    if (document.getElementById('sendbutton').disabled) {
      // Iterate!
      if (cellsOkay()) {
        console.log("All cells okay");
        if (hasMagic()) {
          document.getElementById('sendbutton').disabled = false;
        } else {
          console.log("    but there is no magic word.");
        }
      } else {
        console.log("At least one cell NOT okay");

      }
    }
    updatePlot();  // Maybe only if things are okay, but we don't care about the button.
  }
}

function validateTime(val) {
  // Regexp from https://stackoverflow.com/questions/5563028/how-to-validate-with-javascript-an-input-text-with-hours-and-minutes
  var isValid = /^([0-1]?[0-9]|2[0-4]):([0-5][0-9])?$/.test(val);
  return isValid;
}

/* Verify that all cells requiring a time or temperature contain one. */
function cellsOkay()  {
  const table = document.getElementById('rampTable');
  console.log("Checking cells for good values.");
  // Loop through rows and cells 
  var rval = true;  // Validate all - do not exit after the first bad value.
  for (let row of table.rows) 
  {
      for(let cell of row.cells) 
      {
        //AAA console.log("Looking at cell content " + cell.innerText);
        if (cell.classList.contains("temperature") && !validateTemperature(cell.innerText)) {
    	  cell.classList.add('error');
          cell.style.backgroundColor="#ff4444";  
          rval = false;
        } else if (cell.classList.contains("temperature")) {
          // Most cells will already be clear, but those in added lines benefit from this:
          cell.style.backgroundColor="";  // Clear back to default
        }
        if (cell.classList.contains("time") && !validateTime(cell.innerText)) {
          cell.style.backgroundColor="#ff8800";  
          rval = false;
        } else if (cell.classList.contains("time")) {
          // Most cells will already be clear, but those in added lines benefit from this:
          cell.style.backgroundColor="";  // Clear back to default
        }
      }
  }
  // Should this depend on rval?
  updatePlot();
  return rval;
}

// Check on load in case there are existing bad values, such as when Settings.ini doesn't match NT.
window.onload = cellsOkay;
)rawliteral";

/* 
 * The temperature monitoring page HTML and Javascript.
 */
const char plotlyHTML[] PROGMEM = R"rawliteral(<html>
<head>
	<title>~TITLE~</title>
	<script src="plotly.js" charset="utf-8"></script>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    th, td {
    padding: 15px;
    text-align: left;
    }

    // Note that these do NOT override a fixed size from any inline CSS.
    // @media call order matters.  Set for big, but always override for the smaller case.
    @media (width >= 500px) {
      .fittwowide {
        width: 500px;
        height: 500px;
        background-color: rgb(192, 128, 128);

      }
    }
    @media ((width < 500px) or (height < 500px)) {
      .fittwowide {
        width: 98vmin;
        height: 98vmin;
                background-color: rgb(128,128,192);

      }
    }

  </style>
  <link rel="stylesheet" type="text/css" href="/page.css" />
</head>
<body>
<div class="container">
		
<div id="historyPlot"  class="wrapper flex fittwowide" ></div>
<div id="statusTable"  class="wrapper flex fittwowide" style="height:150px;">
~TABLE_NT~
</div>
<div id="bbb"  class="wrapper flex fittwowide"  style="height:30px;">Nothing yet.</div>

<script>
	TESTER = document.getElementById('historyPlot');
  var counter = 6;
  var data = [];
  var trace;
  for (let i=0; i < ~NT~; i++) {
    //trace = {x: [1, 2, 3, 4, 5],
    //  y: [1, 2, 4, 8, 16],
    trace = {x: [],
      y: [],
      mode: 'lines', name: 'Tank '+(i+1)};
    data.push(trace);
  }

	tHistory = Plotly.newPlot( TESTER, data, {
	margin: { t: 50 },
  title: 'Temperature History',
  xaxis: {title: 'Seconds'},
  yaxis: {title: 'T, '+ String.fromCodePoint(176) + 'C'},  // Just pasting ° didn't work 176 gives the symbol and 8451 gives it with the C.
  responsive: true  // Resize when window resizes, so CSS can fit on different devices.
   } );

  // Get the first actual data point right away, later it will be repeated as defined by setInterval.
  newXY = getData(TESTER);

  setInterval(function() {
    newXY = getData(TESTER);
  }, 10000)

  // must be async to use await.
  async function getData(TESTER) {

    let jjj;
    // XXX IP needs to be dynamic in the future.
    const res = await fetch("http://~IP~/currentT");
    jjj = await res.json();

    console.log("Received NT:");
    console.log(jjj.NT);
    console.log("Received timeval:");
    console.log(jjj.timeval);
    console.log("Received tempList:");
    console.log(jjj.tempList);    
    console.log("Received targetList:");
    console.log(jjj.targetList);
    console.log("Received time of day:");
    console.log(jjj.CBASStod);

    console.log("compare NT vals ", jjj.NT == ~NT~);
    console.log(" is json NT == 3? ", jjj.NT == 3);


    // Don't forget that we receive strings, need numbers!
    newTime = parseInt(jjj.timeval);
    let newTemp = new Array(~NT~);
    for (let i=0; i<~NT~; i++) {
      newTemp[i] = parseFloat(jjj.tempList[i]);
    }
    //newTemp = parseFloat(jjj.tempval);
    document.getElementById('bbb').innerText=jjj.tempList;

    console.log("Extending traces");
   	// Already passed in  TESTER = document.getElementById('tester');

    // Replace hardwired 4-tank code with variable NT tanks.
    var data = [];
    var entry;
    var insideTime = [];
    var insideTemp = [];
    for (let i=0; i < ~NT~; i++) {
      insideTime[i] = [newTime];
      insideTemp[i] = [newTemp[i]];
    }
    data = {x: insideTime, y:insideTemp};
    console.log(data);
 
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
    var listNT = new Array(~NT~);  // note that = [NT] would be an array of length 1 and value NT
    console.log(listNT)
    for (let i=0; i < ~NT~; i++) {
      listNT[i] = i;
    }
    console.log(listNT)
    console.log([listNT])

    //Plotly.extendTraces(TESTER, data, [0, 1, 2], 40);
    Plotly.extendTraces(TESTER, data, listNT, 40);

    // Now update the text information
    let delta = 0.0;
    for (let i=1; i <= ~NT~; i++) {
      document.getElementById('temp' + i).innerHTML=jjj.tempList[i-1];
      delta = parseFloat(jjj.tempList[i-1]) - parseFloat(jjj.targetList[i-1]);
      if (delta > 0.49) {
        document.getElementById('temp' + i).style.backgroundColor="#FF8888";
      } else if (delta < -0.49) {
        document.getElementById('temp' + i).style.backgroundColor="AAAAFF";
      } else {
        document.getElementById('temp' + i).style.backgroundColor="#f8f8f8";
      }
      document.getElementById('set' + i).innerHTML=jjj.targetList[i-1];
    }

    const now = new Date();
    document.getElementById('cbassTime').innerText=`CBASS time: ${jjj.CBASStod}`
    document.getElementById('updateTime').innerText=`Last update: ${now.getHours()}:${String(now.getMinutes()).padStart(2, '0')}`

    return [parseInt(newTime), parseFloat(newTemp)];
  }

  function useData(t) {
    console.log(t);
    document.getElementById('bbb').innerText=t;
    // Note that split does not use alternate delimiters, i.e. ", " means comma then space, not comma or space.
    parts = t.split(",");
    return [parseInt(parts[0]), parseFloat(parts[1])]
  }
</script>
~LINKLIST~
</div>
</body></html>
)rawliteral";


/* 
 * The plot portion of the ramp management page.  HTML and Javascript.
 */
const char rampPlanJavascript[] PROGMEM = R"rawliteral(
  updatePlot();
   function updatePlot() {
    /*
     A trace looks like this. Get new values from the HTML table.
          var trace1 = {x: [1, 2, 3, 4, 5],
          y: [1, 2, 4, 8, 16],
          mode: 'lines', name: 'Tank 1' };

     The data object for the table is an array of such traces.
     */
    const table = document.getElementById('rampTable');

    newData = [];
    for (let t = 0; t < ~NT~; t++) {
      tname = "Tank " + (t+1);
      oneTrace = {x:[], y:[], mode:'lines', name:tname};
      for (let i = 1; i < table.rows.length; i++) {  // Start from 1 to skip header row
        // Just do one tank at a time even though it means multiple passes.
        oneTrace.x[i-1] = table.rows[i].cells[0].innerText.trim();
        oneTrace.y[i-1] = parseFloat(table.rows[i].cells[t+1].innerText.trim());

      }
      newData.push(oneTrace);

      console.log("New trace data:");
      console.log(oneTrace);

    }
    console.log("New trace set:");
    console.log(newData);

    
   	ramp = document.getElementById('rgraph');

    tHistory = Plotly.newPlot( ramp, newData, {
    margin: { t: 50 },
    title: 'Temperature Plan',
    xaxis: {title: 'Time from Start'},
    yaxis: {title: 'T, °C'}
    } );

  }

  )rawliteral";
