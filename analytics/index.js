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
}, function (error, results, fields) {
    if (error) console.log(error);

    total_distance = 0.0;
    total_time = 0.0;
    for (let i = 0; i < results.length; i++) {
        total_distance += results[i].distance_meters;
        total_time += results[i].duration;
    }
    let min_date = new Date(results[0].start_time);

    console.log(`Calculating analytics for all data since ${min_date}`);
    console.log("Total distance:\n\t", total_distance.toPrecision(8), "(meters)\n\t ",
                (total_distance / 1609.34).toPrecision(6), "(mi)");
    console.log("Total time:\n\t", total_time.toPrecision(10), "(ms)\n\t",
                (total_time/1000/60/60).toPrecision(5), "(hours)");
    console.log("Mean speed:\n\t",
                (total_distance / (total_time/1000)).toPrecision(4), "(meters/sec)\n\t",
                ((total_time/1000/60) / (total_distance/1609.34)).toPrecision(4), "(min/mi)");
    connection.end();
});
