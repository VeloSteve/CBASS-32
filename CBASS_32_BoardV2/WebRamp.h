// The Ramp Management page is a bit of a special case and has a lot of code. 
// WebRamp.h (this file) will be just for it, with other things in WebPieces.h

/* A page for starting a reset of the ramp plan.  Normally only 
 * used when an existing file is lost or a clean start is desired. 
 */
const char iniResetPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
	<title>~TITLE~</title>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" type="text/css" href="/page.css" />
</head>
<body>
<div id="message" class="flex" style="color:red; height: fit-content;">~ERROR_MSG~</div>

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


/* Javascript for collecting data from a ramp plan table. 
 * - as table cells are changed confirm that values are valid, if not mark them and disable the send button
 * - allow table rows to be inserted or deleted (keep at least 2)
 * - collect the data and send to the server
 * - notify the user on success or failure
 */
const char rampPlanJavascript[] PROGMEM = R"rawliteral(
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
    oneTrace = {x:[], y:[], mode:'lines', name:tname, type: 'scatter'};
    for (let i = 1; i < table.rows.length; i++) {  // Start from 1 to skip header row
      // Just do one tank at a time even though it means multiple passes.
      // This adds an arbitrary date to the front of the time, because plotly
	    // won't understand that this is a date-time value without it.
      oneTrace.x[i-1] = '2024-01-01T' + table.rows[i].cells[0].innerText.trim();
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
  xaxis: {title: 'Time from Start', tickformat: '%H:%M},
  yaxis: {title: 'T, °C'}
  }, {modeBarButtonsToRemove: ['resetScale2d']} );

}

  )rawliteral";