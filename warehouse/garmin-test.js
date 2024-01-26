require('dotenv').config({ path: '/home/pi/Documents/run-logger/warehouse/.env' });
const { GarminConnect } = require('garmin-connect');
const GCClient = new GarminConnect();

let dates = [
    "2021-10-16",
    "2021-10-15",
    "2021-10-13",
    "2021-10-12",
    "2021-10-10",
    "2021-10-09",
    "2021-10-08",
    "2021-10-04",
    "2021-10-02",
    "2021-10-01",
    "2021-09-30",
    "2021-09-29",
    "2021-09-28",
    "2021-09-28",
    "2021-09-27",
    "2021-09-23",
    "2021-09-18",
    "2021-09-17",
    "2021-09-16",
    "2021-09-15",
    "2021-09-14",
    "2021-09-13",
    "2021-09-11",
    "2021-09-09",
    "2021-09-09",
    "2021-09-08",
    "2021-09-07",
    "2021-09-07",
];

async function auth() {
    await GCClient.login(process.env.GARMIN_USER, process.env.GARMIN_PASS);
    const userInfo = await GCClient.getUserInfo();
    const activities = await GCClient.getActivities();
    let newActivities = activities.map(activity => {
	let calendarEvent = createCalendarEvent(activity);
    });
}

function createCalendarEvent(date) {
    const METERS_IN_MILE = 1609.34;
    let start_date = new Date(date);
    let end_date = new Date(date);
    end_date.setDate(start_date.getDate() + 1);
    console.log(start_date, end_date);
/*    return {
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
*/
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

// auth();
dates.map(date => createCalendarEvent(date));
