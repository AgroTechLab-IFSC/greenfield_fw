function delayRedirect(){
    document.getElementById('delayMsg').innerHTML = 'Please wait while the device is restarting! You\'ll be redirected after <span id="countDown">15</span> seconds...';
    var count = 15;
    setInterval(function(){
        count--;
        document.getElementById('countDown').innerHTML = count;
        if (count == 2) {
            window.location = window.location.href; 
        }
    },1000);
}

function getConfJSONFile(){
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/api/v1/system/conf", true);
    xhr.responseType = 'json';
    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4 && xhr.status === 200) {
            var jsonResponse = xhr.response;            
            const link = document.createElement("a");            
            const file = new Blob([JSON.stringify(jsonResponse, null, 2)], { type: 'application/json' });
            link.href = URL.createObjectURL(file);
            link.download = "greenfield_config.json";
            link.click();
            URL.revokeObjectURL(link.href);
        }
    };
    xhr.send();
}