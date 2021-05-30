require('dotenv').config();
const fs = require('fs');
let request = require('request');
const { v4: uuidv4 } = require('uuid');

const {google} = require('googleapis');
const auth = new google.auth.GoogleAuth({
    keyFile: process.env.GOOGLE_AUTH_KEY_PATH,
    scopes: ['https://www.googleapis.com/auth/calendar'],
});

const readline = require('readline');
let mysql = require('mysql');
let connection = mysql.createConnection({
    host     : process.env.MYSQL_HOST,
    user     : process.env.MYSQL_USER,
    password : process.env.MYSQL_PASS,
    database : process.env.MYSQL_DB,
    port     : process.env.MYSQL_PORT
});
connection.connect();

///////////////////////////////////////
var express = require('express');
var app = express();
var path = require('path');

// viewed at http://localhost:8080
app.get('/*', function(req, res) {
    res.sendFile(path.join(__dirname + '/googlec2523245017e1126.html'));
});

app.listen(process.env.PORT || 8080);
///////////////////////////////////////

const SCOPES = ['https://www.googleapis.com/auth/calendar'];
const TOKEN_PATH = 'token.json';

fs.readFile('credentials.json', (err, content) => {
    if (err) return console.log('Error loading client secret file:', err);

    authorize(JSON.parse(content), watchCalendar);
    /*
    setInterval(() => {
        authorize(JSON.parse(content), watchCalendar);
    }, 1000*60*60*24*7);

    
    authorize(JSON.parse(content), storeNewEvents);
    setInterval(() => {
        authorize(JSON.parse(content), storeNewEvents);
    }, 1000*60*60);
    */
    app.post('/*', (req, res) => {
        console.log('POST REQ');
        authorize(JSON.parse(content), storeNewEvents);
    });
    app.get('/*', (req,res,next) => {
	console.log('GET REQ');
	authorize(JSON.parse(content), storeNewEvents);
    });

});

/**
 * Create an OAuth2 client with the given credentials, and then execute the
 * given callback function.
 * @param {Object} credentials The authorization client credentials.
 * @param {function} callback The callback to call with the authorized client.
 */
function authorize(credentials, callback) {
  const {client_secret, client_id, redirect_uris} = credentials.web;//credentials.installed;
  const oAuth2Client = new google.auth.OAuth2(
      client_id, client_secret, redirect_uris[0]);

  // Check if we have previously stored a token.
  fs.readFile(TOKEN_PATH, (err, token) => {
    if (err) return getAccessToken(oAuth2Client, callback);
    oAuth2Client.setCredentials(JSON.parse(token));
    callback(oAuth2Client);
  });
}

/**
 * Get and store new token after prompting for user authorization, and then
 * execute the given callback with the authorized OAuth2 client.
 * @param {google.auth.OAuth2} oAuth2Client The OAuth2 client to get token for.
 * @param {getEventsCallback} callback The callback for the authorized client.
 */
function getAccessToken(oAuth2Client, callback) {
  const authUrl = oAuth2Client.generateAuthUrl({
    access_type: 'offline',
    scope: SCOPES,
  });
  console.log('Authorize this app by visiting this url:', authUrl);
  const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
  });
  rl.question('Enter the code from that page here: ', (code) => {
    rl.close();
    oAuth2Client.getToken(code, (err, token) => {
      if (err) return console.error('Error retrieving access token', err);
      oAuth2Client.setCredentials(token);
      // Store the token to disk for later program executions
      fs.writeFile(TOKEN_PATH, JSON.stringify(token), (err) => {
        if (err) return console.error(err);
        console.log('Token stored to', TOKEN_PATH);
      });
      callback(oAuth2Client);
    });
  });
}

/**
 * Format events and insert them into MySQL DB
 * @param {google.auth.OAuth2} auth An authorized OAuth2 client.
 */
function storeAllEvents(auth) {
  const calendar = google.calendar({version: 'v3', auth});
  const start_date = new Date('January 1, 1970 00:00:00');
  calendar.events.list({
    calendarId: 'jhkkf4eh49laqptu1i4esd8d8o@group.calendar.google.com',
    timeMin: start_date.toISOString(),
    maxResults: 10000,
    singleEvents: true,
    orderBy: 'startTime',
  }, (err, res) => {
    if (err) return console.log('The API returned an error: ' + err);
    const events = res.data.items;
    if (events.length) {
        events.filter((event) => {
            return event.summary.includes('running');
        }).map((event, i) => {
            const start = event.start.dateTime || event.start.date;
            //let start_date = new Date(start);
            //console.log(start_date);
            console.log(`${start} - ${event.summary}`);
            let row = event.summary.split('/').map(s => s.trim());
            if (row.length < 3
                || !row[0].includes('running')
                || !row[1].includes('mi')
                || !row[2].includes('min'))
                return;
            let distance = row[1];
            let time = row[2];

            distance = parseFloat(distance.replace(/[~mi!ish]/g, ''));
            if (isNaN(distance)) return;
            distance = distance * 1609.34; // convert from miles to meters

            let hour = 0;
            let min = 0;
            let sec = 0;
            if (time.includes('hr')) {
                hour = parseFloat(time.substr(0,time.indexOf('hr')));
                min = parseFloat(time.substr(time.indexOf('hr')+2,time.indexOf('min')));
            }
            else {
                min = parseFloat(time.substr(0,time.indexOf('min')));
            }
            sec = parseFloat(time.substr(time.indexOf('min')+3,time.indexOf('s')));

            if (isNaN(hour) || isNaN(min) || isNaN(sec)) return;
            let total_millis = (hour*60*60 + min*60 + sec) * 1000;

            connection.query({
                sql: 'INSERT INTO runlog (date, duration, distance_meters) values (?,?,?)',
                timeout: 40000,
                values: [start, total_millis, distance]
            }, function (error, results, fields) {
                console.log(start, total_millis, distance, results);
            });

        });
        // connection.end();
    } else {
        console.log('No events found.');
        // connection.end();
    }
  });
}

/**
 * Format only new events since last update and insert them into MySQL DB
 * @param {google.auth.OAuth2} auth An authorized OAuth2 client.
 */
function storeNewEvents(auth) {
    console.log(`${(new Date).toISOString()}: Checking for new events`);
    const calendar = google.calendar({version: 'v3', auth});
    connection.query({
        sql: 'SELECT MAX(date) FROM runlog;',
        timeout: 40000
    }, function (error, results, fields) {
        if (error) throw error;
        let start_date = new Date(results[0]['MAX(date)']);
        console.log("latest time in DB: ", start_date);
        start_date.setDate(start_date.getDate() + 1);

        calendar.events.list({
            calendarId: 'jhkkf4eh49laqptu1i4esd8d8o@group.calendar.google.com',
            timeMin: start_date.toISOString(),
            maxResults: 10000,
            singleEvents: true,
            orderBy: 'startTime',
        }, (err, res) => {
            if (err) return console.log('The API returned an error: ' + err);
            const events = res.data.items;
            if (events.length) {
                events.filter((event) => {
                    return event.summary.includes('running');
                }).map((event, i) => {
                    const start = event.start.dateTime || event.start.date;
                    //let start_date = new Date(start);
                    //console.log(start_date);
                    console.log(`${start} - ${event.summary}`);
                    let row = event.summary.split('/').map(s => s.trim());
                    if (row.length < 3
                        || !row[0].includes('running')
                        || !row[1].includes('mi')
                        || !row[2].includes('min'))
                        return;
                    let distance = row[1];
                    let time = row[2];

                    distance = parseFloat(distance.replace(/[~mi!ish]/g, ''));
                    if (isNaN(distance)) return;
                    distance = distance * 1609.34; // convert from miles to meters

                    let hour = 0;
                    let min = 0;
                    let sec = 0;
                    if (time.includes('hr')) {
                        hour = parseFloat(time.substr(0,time.indexOf('hr')));
                        min = parseFloat(time.substr(time.indexOf('hr')+2,time.indexOf('min')));
                    }
                    else {
                        min = parseFloat(time.substr(0,time.indexOf('min')));
                    }
                    sec = parseFloat(time.substr(time.indexOf('min')+3,time.indexOf('s')));

                    if (isNaN(hour) || isNaN(min) || isNaN(sec)) return;
                    let total_millis = (hour*60*60 + min*60 + sec) * 1000;

                    connection.query({
                        sql: 'INSERT INTO runlog (date, duration, distance_meters) values (?,?,?)',
                        timeout: 40000,
                        values: [start, total_millis, distance]
                    }, function (error, results, fields) {
                        console.log(results);
                    });

                });
                // connection.end();
            } else {
                console.log('No new events found.');
                //connection.end();
            }
        });
    });
}

async function watchCalendar(auth) {
    let id = uuidv4();
    const response = await google.calendar({ version: 'v3', auth }).events.watch({
        calendarId: 'jhkkf4eh49laqptu1i4esd8d8o@group.calendar.google.com',
        resource: {
            id,
            address: process.env.WEBHOOK_CB,
            type: 'web_hook',
            params: {
                ttl: '30000',
            },
        },
    }).catch((err) => {
        console.log(err)
    });
    console.log('WATCH RES', response);
}
