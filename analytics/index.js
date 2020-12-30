require('dotenv').config();
var plotly = require('plotly')(process.env.PLOTLY_USER, process.env.PLOTLY_KEY);
var mysql = require('mysql');
var connection = mysql.createConnection({
    host     : process.env.MYSQL_HOST,
    user     : process.env.MYSQL_USER,
    password : process.env.MYSQL_PASS,
    database : process.env.MYSQL_DB
});
connection.connect();
connection.query({
    sql: 'SELECT * FROM runlog ORDER BY date',
    timeout: 40000,
    values: []
}, (error, results, fields) => {
    if (error) console.log(error);

    plot(results);

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
                `${Math.floor(mean_speed_minpermile)}:${(mean_speed_minpermile%1*60).toPrecision(4)} min/mi`);
    console.log("Mean miles per week:\n\t",
                mean_mpw.toPrecision(4), "mi");

    // connection.end();
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
        let distance = (results[i].distance_meters/1609.34).toPrecision(3);
        let speed = ((results[i].duration/1000/60)/(results[i].distance_meters/1609.34));
        console.log("\t",
                    new Date(results[i].date).toLocaleDateString(), "-",
                    distance, "miles at",
                    `${Math.floor(speed)}:${(speed%1*60).toPrecision(4)}`,
                    "min/mi");
    }
    connection.end();
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
    let mean_speed = (median_min/median_miles);
    console.log(`Median distance of the past 7 days: ${median_miles} mi`);
    console.log(`Median speed of the past 7 days: ${Math.floor(mean_speed)}:${(mean_speed%1*60).toPrecision(4)} min/mi`);
    //connection.end();
});

connection.query({
    sql: 'SELECT SUM(distance_meters)/1609.34 from (SELECT * FROM runlog WHERE EXTRACT(YEAR FROM date) = EXTRACT(YEAR FROM NOW())) year_runs;',
    timeout: 40000,
    values: []
}, (error, results, fields) => {
    if (error) throw error;
    console.log("Miles so far this year:");
    console.log("\t", (results[0]['SUM(distance_meters)/1609.34']).toPrecision(4), "mi");
});

function plot(results) {
        let dates = [];
        let distances = [];
        let mpw_dates = [];
        let mpw = [];
        for (let i = 0; i < results.length; i++) {
            dates.push(results[i].date);
            distances.push(results[i].distance_meters/1609.34);
        }

        let beginning = new Date(results[0].date);
        beginning.setDate(beginning.getDate() - (beginning.getDay() + 7) % 7);
        let end = new Date(results[results.length-1].date);
        end.setDate(end.getDate() - (end.getDay() + 7) % 7);
        let i = new Date(beginning);
        while (i < end) {
            i.setDate(i.getDate() + 7);
            let mpw_period_date = new Date(i);
            let current_mpw = 0;
            for (let j = 0; j < 7; j++) {
                let target_date = new Date(i);
                target_date.setDate(target_date.getDate() + j);
                let runs = results.filter((r) => {
                    current_date = new Date(r.date);
                    return current_date.getYear() == target_date.getYear()
                        && current_date.getMonth() == target_date.getMonth()
                        && current_date.getDate() == target_date.getDate();
                });
                for (let r = 0; r < runs.length; r++) {
                    if (runs[r] && runs[r].distance_meters) {
                        current_mpw += runs[r].distance_meters / 1609.34;
                    }
                }
            }

            if (current_mpw != 0) {
                mpw_dates.push(mpw_period_date);
                mpw.push(current_mpw);
            }
        }

        let distance_data = {
            x: dates,
            y: distances,
            mode: "markers",
            type: "scatter",
            name: "Distance",
        };
        let mpw_data = {
            x: mpw_dates,
            y: mpw,
            mode: "markers",
            type: "scatter",
            name: "Miles Per Week",
        };
        let data = [distance_data, mpw_data];
        let graph_options = {
            filename: "date-axes",
            fileopt: "overwrite",
        };
        plotly.plot(data, graph_options, (err, msg) => {
            // console.log(msg);
            if (err) throw err;
        });
    }  // END PLOT
