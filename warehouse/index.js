require('dotenv').config();
const fs = require('fs');
const {google} = require('googleapis');
const readline = require('readline');
var mysql = require('mysql');
var connection = mysql.createConnection({
    host     : process.env.MYSQL_HOST,
    user     : process.env.MYSQL_USER,
    password : process.env.MYSQL_PASS,
    database : process.env.MYSQL_DB
});
connection.connect();

const SCOPES = ['https://www.googleapis.com/auth/calendar'];
const TOKEN_PATH = 'token.json';

fs.readFile('credentials.json', (err, content) => {
  if (err) return console.log('Error loading client secret file:', err);
    authorize(JSON.parse(content), storeNewEvents);
});

/**
 * Create an OAuth2 client with the given credentials, and then execute the
 * given callback function.
 * @param {Object} credentials The authorization client credentials.
 * @param {function} callback The callback to call with the authorized client.
 */
function authorize(credentials, callback) {
  const {client_secret, client_id, redirect_uris} = credentials.installed;
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
            let start_date = new Date(start);
            // console.log(start_date);
            // console.log(`${start} - ${event.summary}`);
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
                sql: 'INSERT INTO runlog (start_time, duration, distance_meters) values (?,?,?)',
                timeout: 40000,
                values: [start_date, total_millis, distance]
            }, function (error, results, fields) {

            });
        });
    } else {
        console.log('No events found.');
    }
  });
}


/**
 * Format only new events since last update and insert them into MySQL DB
 * @param {google.auth.OAuth2} auth An authorized OAuth2 client.
 */
function storeNewEvents(auth) {
    const calendar = google.calendar({version: 'v3', auth});
    connection.query({
        sql: 'SELECT MAX(start_time) FROM runlog;',
        timeout: 40000
    }, function (error, results, fields) {
        if (error) throw error;
        let start_date = new Date(results[0]['MAX(start_time)']);
        console.log("latest time in DB: ", start_date);

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
                    let start_date = new Date(start);
                    console.log(start_date);
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
                        sql: 'INSERT INTO runlog (start_time, duration, distance_meters) values (?,?,?)',
                        timeout: 40000,
                        values: [start_date, total_millis, distance]
                    }, function (error, results, fields) {

                    });
                });
            } else {
                console.log('No events found.');
            }
        });
    });
}
