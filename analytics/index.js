require('dotenv').config();
var mysql = require('mysql');
var connection = mysql.createConnection({
    host     : process.env.MYSQL_HOST,
    user     : process.env.MYSQL_USER,
    password : process.env.MYSQL_PASS,
    database : process.env.MYSQL_DB
});
connection.connect();
connection.query({
    sql: 'SELECT * FROM runlog',
    timeout: 40000,
    values: []
}, (error, results, fields) => {
    if (error) console.log(error);

    let total_distance = 0.0;
    let total_time = 0.0;
    let min_date = new Date(results[0].date);
    let max_date = new Date(results[0].date);
    for (let i = 0; i < results.length; i++) {
        total_distance += results[i].distance_meters;
        total_time += results[i].duration;

        if (results[i].date < min_date)
            min_date = new Date(results[i].date);
        if (results[i].date > max_date)
            max_date = new Date(results[i].date);
    }

    let total_miles = (total_distance / 1609.34);
    let total_ms = (total_time/1000/60/60);
    let mean_speed_mps = (total_distance / (total_time/1000));
    let mean_speed_minpermile = ((total_time/1000/60) / (total_distance/1609.34));
    let mean_mpw = total_miles / ((max_date - min_date)/(1000*60*60*24*7));

    console.log(`Calculating analytics for all data since ${min_date}`);
    console.log("Total distance:\n\t", total_distance.toPrecision(8), "meters\n\t ",
                total_miles.toPrecision(6), "mi");
    console.log("Total time:\n\t",
                total_ms.toPrecision(5), "hours");
    console.log("Mean speed:\n\t",
                mean_speed_mps.toPrecision(4), "meters/sec\n\t",
                mean_speed_minpermile.toPrecision(4), "min/mi");
    console.log("Mean miles per week:\n\t",
                mean_mpw.toPrecision(4), "mi");

    // connection.end();
});

connection.query({
    sql: 'SELECT * FROM runlog WHERE date >= (NOW() - INTERVAL 7 DAY);',
    timeout: 40000,
    values: []
}, (error, results, fields) => {
    if (error) throw error;
    console.log("MPW for the past 7 days:");
    let mpw = 0;
    for (let i = 0; i < results.length; i++) {
        mpw += (results[i].distance_meters / 1609.34);
    }
    console.log("\t", mpw.toPrecision(4), "mpw");

    let sorted_distances = results.map((row) => {
        return row.distance_meters/1609.34
    }).sort();
    let sorted_times = results.map((row) => {
        return row.duration/1000/60;
    }).sort();
    let median_miles = sorted_distances[Math.floor(sorted_distances.length/2)];
    let median_min = sorted_times[Math.floor(sorted_times.length/2)];
    console.log(`Median distance of the past 7 days: ${median_miles} mi`);
    console.log(`Median speed of the past 7 days: ${(median_min/median_miles).toPrecision(4)} min/mi`);
    //connection.end();
});

connection.query({
    sql: 'SELECT distance_meters,duration,date FROM runlog ORDER BY distance_meters DESC, duration ASC LIMIT 10;',
    timeout: 40000,
    values: []
}, (error, results, fields) => {
    if (error) throw error;
    console.log("Your 10 longest runs were:");
    for (let i = 0; i < results.length; i++) {
        console.log("\t",new Date(results[i].date).toLocaleDateString(), "-",
                    (results[i].distance_meters/1609.34).toPrecision(3),"mi");
    }
    // connection.end();
});

connection.query({
    sql: 'SELECT * FROM runlog ORDER BY (distance_meters/duration) desc limit 10;',
    timeout: 40000,
    values: []
}, (error, results, fields) => {
    if (error) throw error;
    console.log("Your 10 fastest runs were:");
    for (let i = 0; i < results.length; i++) {
        console.log("\t",
                    new Date(results[i].date).toLocaleDateString(), "-",
                    (results[i].distance_meters/1609.34).toPrecision(3), "miles at",
                    ((results[i].duration/1000/60)/(results[i].distance_meters/1609.34)).toPrecision(3),
                    "min/mi");
    }
    connection.end();
});
