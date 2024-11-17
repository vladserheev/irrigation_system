window.zonesWateringData = [];
window.sensorsData = [];

$(window).on("dataReceived", (event, data) => {
    console.log(event);
    console.log(data);
    data.zonesWateringData.forEach(d => {
        d.start = new Date(d.start);
        d.end= new Date(d.end);

    });
    data.sensorsData.forEach(d => {
        d.date = new Date(d.date);
    });
    window.zonesWateringData = data.zonesWateringData;
    window.sensorsData = data.sensorsData;
    $('#dateRangeInput').trigger('change');
});

$('#dateMinusDay').click(() => {
    let currentDate = $('#dateRangeInput').val();
    if (currentDate) {
        let dateFromInput = new Date(currentDate);
        dateFromInput.setDate(dateFromInput.getDate() - 1);
        let newDate = dateFromInput.toISOString().split('T')[0];
        $('#dateRangeInput').val(newDate);
        $('#dateRangeInput').trigger('change');
    }
})
$('#datePlusDay').click(() => {
    let currentDate = $('#dateRangeInput').val();
    if (currentDate) {
        let dateFromInput = new Date(currentDate);
        dateFromInput.setDate(dateFromInput.getDate() + 1);
        let newDate = dateFromInput.toISOString().split('T')[0];
        $('#dateRangeInput').val(newDate);
        $('#dateRangeInput').trigger('change');
    }
})
$("#dateRangeInput").on("change", () =>{
    console.log("change");
    console.log(window.zonesWateringData);

    const date = new Date($("#dateRangeInput").val()); // Current date, adjust if needed.
    dateRange.startDate = new Date(date.setHours(0, 0, 0, 0)); // Start of the day.
    dateRange.endDate = new Date(date.setHours(23, 59, 59, 999)); // End of the day.

    const filteredData = window.sensorsData.filter(d => new Date(d.date) >= dateRange.startDate && new Date(d.date) <= dateRange.endDate);
    const filteredGantt = window.zonesWateringData.filter(d => new Date(d.start) >= dateRange.startDate);

    console.log("filtred Sensors data:", filteredData);
    console.log("filtred Watering zones data:", filteredGantt);
    update(dateRange, filteredData, filteredGantt);
})

const rgbColors = {
    "airHumidity": "#FFCD56",
    "soilHumidity": "#FE6D73",
    "temperature": "#36a2eb"
}
dateRange = {}

// Filter the data array based on the date range
//const filteredData = sensorsData.filter(d => d.date >= dateRange.startDate && d.date <= dateRange.endDate);

const margin = { top: 20, right: 50, bottom: 50, left: 50 };
const width = 600 - margin.left - margin.right;
const height = 400 - margin.top - margin.bottom;

$("#errorBlock").css({"width":width + margin.left, "height":height + margin.top});

const svg = d3.select("#chart")
    .append("svg")
    .attr("width", width + margin.left + margin.right)
    .attr("height", height + margin.top + margin.bottom)
    .append("g")
    .attr("transform", `translate(${margin.left},${margin.top})`);

// Масштабы для осей X и Y
dateRangeArray = [dateRange.startDate, dateRange.endDate];

const xScale = d3.scaleTime()
    //.domain(dateRangeArray)
    .range([0, width]);

const yScaleTemp = d3.scaleLinear()
    .domain([0, 10])
    .range([height, 0]);
const yScaleHum = d3.scaleLinear()
    .domain([0, 10])
    //.domain([0, d3.max(sensorsData, d => Math.max(d.temperature, d.soilHumidity, d.airHumidity))])
    .range([height, 0]);

svg.append("g")
    .attr("transform", `translate(${width}, 0)`)
    .attr("class", "temp-axis")
    .call(d3.axisRight(yScaleTemp).ticks(10))
    .selectAll(".tick text")
    .attr("class", "axis-label")

svg.append("g")
    .attr("transform", `translate(0, 0)`)
    .attr("class", "hum-axis")
    .call(d3.axisLeft(yScaleHum).ticks(10))
    .selectAll(".tick text")
    .attr("class", "axis-label");

// Ось X для времени
svg.append("g")
    .attr("transform", `translate(0,${height})`)
    //.style("stroke", "#227c9d")
    .call(d3.axisBottom(xScale).tickFormat(d3.timeFormat('%H:%M')).ticks(d3.timeHour.every(2)))
    .selectAll(".tick text")
    //.style("stroke", "#227c9d")
    .attr("class", "axis-label");

svg.append("text")
    .attr("class", "label")
    .attr("class", "y label")
    //.attr("text-anchor", "end")
    .attr("transform", "rotate(-90)")
    .attr("y", width+margin.left-7)
    .attr("x", -margin.top-185)
    .attr("transform", "rotate(-90)")
    .text("Temperature [°C]");

svg.append("text")
    .attr("class", "label")
    .attr("class", "y label")
    .attr("text-anchor", "end")
    .attr("transform", "rotate(-90)")
    .attr("y", -margin.left+20)
    .attr("x", -margin.top-100)
    //.attr("dy", ".75em")
    .attr("transform", "rotate(-90)")
    .text("Humidity [%]");


svg.append("text")
    .attr("class", "label")
    .attr("class", "y label")
    //.attr("text-anchor", "middle")
    .attr("y", height+margin.top+20)
    .attr("x", width/2-20)
    //.attr("dy", ".75em")
    //.attr("transform", "rotate(-90)")
    .text("Time");

// Ось Y для температуры


svg.selectAll(".x-grid")
    .data(xScale.ticks(d3.timeHour.every(2)))  // Check if you need to slice or use the entire array
    .join("line")
    .attr("class", "x-grid")
    .attr("x1", d => xScale(d))
    .attr("x2", d => xScale(d))
    .attr("y1", 0)
    .attr("y2", height)
    .attr("stroke", "#e0e0e0")
    .attr("stroke-width", 0.5);

svg.selectAll(".y-grid")
    .data(yScaleTemp.ticks().slice(1))  // Check if you need to slice or use the entire array
    .join("line")
    .attr("class", "x-grid")
    .attr("y1", d => yScaleTemp(d))
    .attr("y2", d => yScaleTemp(d))
    .attr("x1", 0)
    .attr("x2", width)
    .attr("stroke", "#e0e0e0")
    .attr("stroke-width", 0.5);

// Добавление диаграммы Ганта (Зоны полива)
function update(date_range, data, dataGantt) {
    //console.log($(".errorBlock").length);

    console.log(data);
    if(data == ""){
        console.error("There is no data for this time range!");
        if($(".errorBlock").length == 0){
            svg.append('rect')
                .attr('width', width + margin.left + margin.right)
                .attr('height', height + margin.top + margin.bottom)
                .attr('fill', "black")
                .attr('class', "errorBlock")
                .style("opacity", 0)
                .transition()
                .duration(1000)
                .ease(d3.easeCubicInOut)  // Ease function for smooth transition
                .style("opacity", 0.5)  // Animate to full opacity
                .attr("width", width + margin.left + margin.right * 1.1)  // Animate width change
                .attr("height", height + margin.top + margin.bottom * 1.1);
        }
        return;
    }else{
        svg.selectAll('.errorBlock')
            //.style("opacity", 0.5)
            .transition()
            .duration(1000)
            .ease(d3.easeCubicInOut)  // Ease function for smooth transition
            .style("opacity", 0)
            .remove()
    }

    console.log(date_range);
    arr=[date_range.startDate, date_range.endDate];


    // svg.selectAll(".line").remove();
    xScale.domain(arr);
    yScaleTemp.domain([0, d3.max(data, d => Math.max(d.temperature))]);
    yScaleHum.domain([0, d3.max(data, d => Math.max(d.soilHumidity, d.airHumidity))]);
    //console.log(d3.max(data, d => Math.max(d.temperature, d.soilHumidity, d.airHumidity)));



    svg.select(".temp-axis")
        .transition()
        .duration(1000)
        .call(d3.axisRight(yScaleTemp).ticks(10))
        .selectAll(".tick text")
        .attr("class", "axis-label")

    svg.select(".hum-axis")
        .transition()
        .duration(1000)
        .call(d3.axisLeft(yScaleHum).ticks(10))
        .selectAll(".tick text")
        .attr("class", "axis-label")


    svg.select(".x-axis")
        .transition()
        .duration(1000)
        .call(d3.axisBottom(xScale));


    svg.selectAll(".gantt-task")
        .transition()
        .duration(1000);

    svg.selectAll(".gantt-task")
        .data(dataGantt)
        .join(
            enter => enter.append("rect")
                .attr("class", "gantt-task")
                .attr("x", d => xScale(d.start))
                .attr("y", 0)
                .attr("width", 0)  // Start with width 0 for animation
                .attr("height", height)
                .attr("rx", 5)
                .attr("ry", 5)
                .attr("fill", (d, i) => d.zone === 'Zone 1' ? '#17c3b2' : '#227C9D')
                .attr("width", d => xScale(d.end) - xScale(d.start))
                .style('opacity', 0)
                .lower()
                .call(enter => enter.transition()
                        .ease(d3.easeCubicInOut)
                        .duration(500)
                        .style('opacity', 0.5)
                    //.attr("width", d => xScale(d.end) - xScale(d.start))  // Animate to final width
                )
                .on("mouseover", function(event, d) {
                    d3.select(this).attr("r", 5);
                    d3.select("#tooltip")
                        .style("left", `${event.pageX + 10}px`)
                        .style("top", `${event.pageY + 10}px`)
                        .style("opacity", 1)
                        .text(`${d.zone}: from ${d3.timeFormat("%H:%M")(d.start)} to ${d3.timeFormat("%H:%M")(d.end)}`);
                })
                .on("mouseout", function() {
                    d3.select(this).attr("r", 4);
                    d3.select("#tooltip").style("opacity", 0);
                }),

            update => update
                .transition()
                .ease(d3.easeCubicInOut)
                .duration(1000)
                .attr("x", d => xScale(d.start))
                .attr("width", d => xScale(d.end) - xScale(d.start))
                .attr("fill", (d, i) => d.zone === 'Zone 1' ? '#17c3b2' : '#227C9D'),

            exit => exit
                .transition()
                .ease(d3.easeCubicInOut)
                .duration(500)
                .style('opacity', 0)
                .remove()
        );

    // Линия для графика температуры с интерполяцией
    const lineTemp = d3.line()
        .x(d => xScale(d.date))
        .y(d => yScaleTemp(d.temperature))
        .curve(d3.curveMonotoneX);
    // Используем интерполяцию monotoneX для плавных кривых

    const lineSoilHum = d3.line()
        .x(d => xScale(d.date))
        .y(d => yScaleHum(d.soilHumidity))
        .curve(d3.curveMonotoneX);

    const lineAirHum = d3.line()
        .x(d => xScale(d.date))
        .y(d => yScaleHum(d.airHumidity))
        .curve(d3.curveMonotoneX);

    svg.selectAll(".temperature-line")
        .data([data])
        .transition()
        .duration(1000)  // Set duration for animation
        .attr("d", lineTemp);

    svg.selectAll(".temperature-line")
        .data([data])
        .enter()
        .append("path")
        .attr("class", "temperature-line line")
        .attr("d", lineTemp)
        .style("stroke", rgbColors.temperature)
        .style("fill", "none")
        .transition()
        .duration(1000)  // Apply smooth transition for line creation
        .attr("d", lineTemp);

    svg.selectAll(".soilHum-line")
        .data([data])
        .transition()
        .duration(1000)  // Set duration for animation
        .attr("d", lineSoilHum);

    svg.selectAll(".soilHum-line")
        .data([data])
        .enter()
        .append("path")
        .attr("class", "soilHum-line line")
        .attr("d", lineSoilHum)
        .style("stroke", rgbColors.soilHumidity)
        .style("fill", "none")
        .transition()
        .duration(1000)  // Apply smooth transition for line creation
        .attr("d", lineSoilHum);

    svg.selectAll(".airHum-line")
        .data([data])
        .transition()
        .duration(1000)  // Set duration for animation
        .attr("d", lineAirHum);

    svg.selectAll(".airHum-line")
        .data([data])
        .enter()
        .append("path")
        .attr("class", "airHum-line line")
        .attr("d", lineAirHum)
        .style("stroke", rgbColors.airHumidity)
        .style("fill", "none")
        .raise()
        .transition()
        .duration(1000)
        .attr("d", lineAirHum);

// Temperature points
    svg.selectAll(".temperature-point")
        .transition()
        .duration(1000); // Set duration for animation

    svg.selectAll(".temperature-point")
        .data(data)
        .join(
            enter => enter.append("circle")
                .attr("class", "temperature-point")
                .attr("cx", d => xScale(d.date))
                .attr("cy", d => yScaleTemp(d.temperature))
                .attr("r", 4)
                .attr("fill", hexToRgba(rgbColors.temperature, 0.7))
                .attr("stroke", rgbColors.temperature)
                .on("mouseover", function(event, d) {
                    d3.select(this).attr("r", 5);
                    d3.select("#tooltip")
                        .style("left", `${event.pageX + 10}px`)
                        .style("top", `${event.pageY + 10}px`)
                        .style("opacity", 1)
                        .text(`Temperature: ${d.temperature}°C, ${d3.timeFormat("%H:%M")(d.date)}`);
                })
                .on("mouseout", function() {
                    d3.select(this).attr("r", 4);
                    d3.select("#tooltip").style("opacity", 0);
                }),
            update => update
                .transition()
                .duration(1000)
                .attr("cx", d => xScale(d.date))
                .attr("cy", d => yScaleTemp(d.temperature)),
            exit => exit.remove()
        );

    // Soil humidity points
    svg.selectAll(".soilHum-point")
        .transition()
        .duration(1000); // Set duration for animation

    svg.selectAll(".soilHum-point")
        .data(data)
        .join(
            enter => enter.append("circle")
                .attr("class", "soilHum-point")
                .attr("cx", d => xScale(d.date))
                .attr("cy", d => yScaleHum(d.soilHumidity))
                .attr("r", 4)
                .attr("fill", hexToRgba(rgbColors.soilHumidity, 0.7))
                .attr("stroke", rgbColors.soilHumidity)
                .on("mouseover", function(event, d) {
                    d3.select(this).attr("r", 5);
                    d3.select("#tooltip")
                        .style("left", `${event.pageX + 10}px`)
                        .style("top", `${event.pageY + 10}px`)
                        .style("opacity", 1)
                        .text(`Soil Humidity: ${d.soilHumidity}%, ${d3.timeFormat("%H:%M")(d.date)}`);
                })
                .on("mouseout", function() {
                    d3.select(this).attr("r", 4);
                    d3.select("#tooltip").style("opacity", 0);
                }),
            update => update
                .transition()
                .duration(1000)
                .attr("cx", d => xScale(d.date))
                .attr("cy", d => yScaleHum(d.soilHumidity)),
            exit => exit.remove()
        );

    // Air humidity points
    svg.selectAll(".airHum-point")
        .transition()
        .duration(1000); // Set duration for animation

    svg.selectAll(".airHum-point")
        .data(data)
        .join(
            enter => enter.append("circle")
                .attr("class", "airHum-point")
                .attr("cx", d => xScale(d.date))
                .attr("cy", d => yScaleHum(d.airHumidity))
                .attr("r", 4)
                .attr("fill", hexToRgba(rgbColors.airHumidity, 0.7))
                .attr("stroke", rgbColors.airHumidity)
                .on("mouseover", function(event, d) {
                    d3.select(this).attr("r", 5);
                    d3.select("#tooltip")
                        .style("left", `${event.pageX + 10}px`)
                        .style("top", `${event.pageY + 10}px`)
                        .style("opacity", 1)
                        .text(`Air Humidity: ${d.airHumidity}%, ${d3.timeFormat("%H:%M")(d.date)}`);
                })
                .on("mouseout", function() {
                    d3.select(this).attr("r", 4);
                    d3.select("#tooltip").style("opacity", 0);
                }),
            update => update
                .transition()
                .duration(1000)
                .attr("cx", d => xScale(d.date))
                .attr("cy", d => yScaleHum(d.airHumidity)),
            exit => exit.remove()
        );
}

function hexToRgba(hex, opacity) {
    // Убираем "#" из HEX
    hex = hex.replace('#', '');

    // Получаем RGB компоненты из HEX
    let r = parseInt(hex.slice(0, 2), 16);
    let g = parseInt(hex.slice(2, 4), 16);
    let b = parseInt(hex.slice(4, 6), 16);

    // Возвращаем цвет в формате rgba
    return `rgba(${r}, ${g}, ${b}, ${opacity})`;
}