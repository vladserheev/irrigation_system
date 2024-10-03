const log = (level, message) => {
    const currentTime = new Date().toLocaleString();
    if(message) {
        if(typeof message === 'object'){
            console.log(`[${currentTime}] [${level}]`);
            console.dir(message);
        }else {
            console.log(`[${currentTime}] [${level}] ${message}`);
        }
    }
};
module.exports = {log};