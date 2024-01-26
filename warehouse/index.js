(async () => {

require('dotenv').config({ path: '/home/pi/Documents/run-logger/warehouse/.env' });
const fs = require('fs');
let request = require('request');
const { v4: uuidv4 } = require('uuid');

const {google} = require('googleapis');
const auth = new google.auth.GoogleAuth({
    keyFile: process.env.GOOGLE_AUTH_KEY_PATH,
    scopes: ['https://www.googleapis.com/auth/calendar'],
});
let googleCredentials;

let vault = require("node-vault")({
    apiVersion: 'v1',
    endpoint: process.env.VAULT_ADDR,
    token: process.env.VAULT_TOKEN,
    requestOptions: {
      strictSSL: false,
    }
});
const db_connection_info = await vault.read(process.env.VAULT_DB_PATH);
const app_config = await vault.read(process.env.VAULT_APP_MOUNT);

const { GarminConnect } = require('garmin-connect');
    
const readline = require('readline');
let mysql = require('mysql');
let pool  = mysql.createPool({
    connectionLimit : 10,
    host     : db_connection_info.data.MYSQL_HOST,
    port     : db_connection_info.data.MYSQL_PORT,
    user     : app_config.data.MYSQL_USER,
    password : app_config.data.MYSQL_PASS,
    database : app_config.data.MYSQL_DB,
});
//connection.connect();

let progress = {};

///////////////////////////////////////
let express = require('express');
let app = express();
let path = require('path');
let helmet = require('helmet');
app.use(helmet());

app.get('/*', (req, res) => {
    console.log(`${(new Date).toISOString()}: GET REQ`);
    res.sendFile(path.join(__dirname + '/googlec2523245017e1126.html'));
});

app.listen(app_config.data.PORT || 8080);
console.log("listening on port", app_config.data.PORT||8080);
///////////////////////////////////////

const SCOPES = ['https://www.googleapis.com/auth/calendar'];
const TOKEN_PATH = path.join(__dirname + '/token.json');

fs.readFile(path.join(__dirname + '/credentials.json'), async (err, content) => {
    if (err) return console.log('Error loading client secret file:', err);
    googleCredentials = JSON.parse(content);

    authorize(googleCredentials, watchCalendar);

    setInterval(() => {
        authorize(googleCredentials, watchCalendar);
    }, 1000*60*60*24*7);

    /*
    authorize(JSON.parse(content), storeNewEvents);
    setInterval(() => {
        authorize(JSON.parse(content), storeNewEvents);
    }, 1000*60*60);
    */

    authorize(googleCredentials, syncGarmin);
    setInterval(async () => {
        authorize(googleCredentials, syncGarmin);
    }, 1000*60*60*6);

    app.post('/sync', (req, res) => {
	console.log(`${(new Date).toISOString()}: POST /sync`);
	authorize(googleCredentials, syncGarmin);
	res.status(200).send();
    });


    app.post('/*', (req, res) => {
        console.log(`${(new Date).toISOString()}: POST REQ`);
        authorize(googleCredentials, storeNewEvents);
	res.status(200).send();
    });
});

/**
 * Create an OAuth2 client with the given credentials, and then execute the
 * given callback function.
 * @param {Object} credentials The authorization client credentials.
 * @param {function} callback The callback to call with the authorized client.
 */
function authorize(credentials, callback) {
  const { client_secret, client_id, redirect_uris } = credentials.web;//credentials.installed;
  const oAuth2Client = new google.auth.OAuth2(
      client_id, client_secret, redirect_uris[0]);

  // Check if we have previously stored a token.
  fs.readFile(TOKEN_PATH, (err, token) => {
    if (err) return getAccessToken(oAuth2Client, callback);
    oAuth2Client.setCredentials(JSON.parse(token));
    callback(oAuth2Client);
  });
}

function createCalendarEvent(garminActivity) {
    const METERS_IN_MILE = 1609.34;
    return {
        'summary': `running / ${(garminActivity.distance / METERS_IN_MILE).toFixed(2)}mi / ${parseTime(garminActivity)}`,
        'description': JSON.stringify({source: "Garmin", id: garminActivity.activityId}),
        'start': {
            'date': garminActivity.startTimeLocal.split(" ")[0],
            'timeZone': 'America/New_York',
        },
        'end': {
            'date': garminActivity.startTimeLocal.split(" ")[0],
            'timeZone': 'America/New_York',
        },
    };
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
          console.log(`${(new Date).toISOString()}: Token stored to`, TOKEN_PATH);
      });
      callback(oAuth2Client);
    });
  });
}

function getLastStoredEventId() {

}

/* Garmin stores distance in meters and duration in seconds */
function parseTime(activity) {
    let hrs = Math.floor(activity.duration/3600);
    let min = Math.floor((activity.duration - hrs*3600)/60);
    let sec = activity.duration - hrs*3600 - min*60;
    if (hrs > 0) {
        return `${hrs}hr ${min}min ${sec.toFixed(2)}s`;
    } else {
        return `${min}min ${sec.toFixed(2)}s`;
    }
}

function pushToCalendar(auth, event) {
    console.log(auth);
    const calendar = google.calendar({version: 'v3', auth});
    calendar.events.insert({
        auth,
        calendarId: app_config.data.GOOGLE_CAL_ID,
        resource: event,
    }, function(err, event) {
        if (err) {
            console.log('There was an error contacting the Calendar service: ' + err);
            return;
        }
        console.log('Event created: %s', event.htmlLink);
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
    calendarId: app_config.data.GOOGLE_CAL_ID,
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

            pool.query({
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

function storeGarminEventInDb(garminActivity) {
    console.log("Storing Garmin event in DB", garminActivity);
    const start = garminActivity.startTimeLocal.split(" ")[0];
    const total_millis = garminActivity.duration * 1000;
    pool.query({
        sql: 'INSERT INTO runlog (date, duration, distance_meters, garmin_activity_id) values (?,?,?,?)',
        timeout: 40000,
        values: [start, total_millis, garminActivity.distance, garminActivity.activityId]
    }, function (error, results, fields) {
        console.log(results);
    });
}

/**
 * Format only new events since last update and insert them into MySQL DB
 * @param {google.auth.OAuth2} auth An authorized OAuth2 client.
 */
function storeNewEvents(auth) {
    console.log(`${(new Date).toISOString()}: Checking for new events`);
    const calendar = google.calendar({version: 'v3', auth});
    pool.query({
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
	    const etag = res.data.etag;
	    if (progress[etag]) {
		return;
	    }
	    progress[etag] = true;
	    setTimeout(() => {delete progress[etag]}, 100000);
            if (events.length) {
                events.filter((event) => {
		                let description = null;
		                try {
                        description = JSON.parse(event.description);
		                } catch (err) {
			                  // console.error(err);
			                  console.log("Calendar event description was empty. Assuming manually created event");
		                }
                    if (description && description.source) {
                        return event.summary.includes('running') && description.source !== "Garmin";
                    } else {
                        return event.summary.includes('running');
                    }
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

                    pool.query({
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

async function syncGarmin() {
    try {
	pool.query({
            sql: 'SELECT MAX(garmin_activity_id) FROM runlog;',
            timeout: 40000,
            values: [],
	}, async (error, results, fields) => {
            let lastStoredEventId = results[0]['MAX(garmin_activity_id)'];
	    const GCClient = new GarminConnect({
		username: app_config.data.GARMIN_USER,
		password: app_config.data.GARMIN_PASS
	    });
	    await GCClient.login();
	    // const userInfo = await GCClient.getUserInfo();
	    const activities = await GCClient.getActivities();
	    let newActivities = [];
	    if (lastStoredEventId && Array.isArray(activities)) {
		newActivities = activities
		    .filter(activity =>
			    activity.activityId > lastStoredEventId
			    && activity.activityType.typeKey === "running")
		    .sort((a1, a2) =>
			  a1.activityId - a2.activityId);
	    } else if(lastStoredEventId) {
		newActivities = [activities];
	    }

	    console.log("new activities:", newActivities);
	    newActivities = newActivities.map(activity => {
		storeGarminEventInDb(activity);
		let calendarEvent = createCalendarEvent(activity);
		authorize(googleCredentials, (auth) => pushToCalendar(auth, calendarEvent));
	    });
	});
    } catch (error) {
	console.error(error);
    }
}

async function watchCalendar(auth) {
    let id = uuidv4();
    const response = await google.calendar({ version: 'v3', auth }).events.watch({
        calendarId: app_config.data.GOOGLE_CAL_ID,
        resource: {
            id,
            address: app_config.data.WEBHOOK_CB,
            type: 'web_hook',
        },
    }).catch((err) => {
        console.log(err)
    });
    console.log(`${(new Date).toISOString()}: WATCH RES`, response);
}

})().catch((err) => { console.error(err) });
