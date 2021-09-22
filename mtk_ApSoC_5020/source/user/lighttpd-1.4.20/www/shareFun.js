function display(expr, div)
{
	if (expr != "none")
		document.getElementById(div).style.visibility = "visible";
	else
		document.getElementById(div).style.visibility = "hidden";
	try 
	{
		document.getElementById(div).style.display = expr;
	}
	catch (e)
	{
		document.getElementById(div).style.display = "block";
	}
}

function atoi(str, num)
{
	i = 1;
	if (num != 1) {
		while (i != num && str.length != 0) {
			if (str.charAt(0) == '.') {
				i++;
			}
			str = str.substring(1);
		}
		if (i != num)
			return -1;
	}

	for (i=0; i<str.length; i++) {
		if (str.charAt(i) == '.') {
			str = str.substring(0, i);
			break;
		}
	}
	if (str.length == 0)
		return -1;
	return parseInt(str, 10);
}

function checkRange(str, num, min, max)
{
	d = atoi(str, num);
	if (d > max || d < min)
		return false;
	return true;
}

function checkMac(str){
    var len = str.length;
    if(len!=17)
        return false;

    for (var i=0; i<str.length; i++) {
        if((i%3) == 2){
            if(str.charAt(i) == ':')
                continue;
        }else{
            if (    (str.charAt(i) >= '0' && str.charAt(i) <= '9') ||
                    (str.charAt(i) >= 'a' && str.charAt(i) <= 'f') ||
                    (str.charAt(i) >= 'A' && str.charAt(i) <= 'F') )
                continue;
        }
        return false;
    }
    return true;
}

function checkIpAddr(field, ismask)
{
	var ip_addr = field.value.split(".");
	var i;

	if (field.value == "")
	{
		alert("Error. IP address is empty.");
		field.value = field.defaultValue;
		field.focus();
		return false;
	}
	if (ip_addr.length != 4)
	{
			alert('IP adress format error, please keyin like 10.10.10.254.');
			field.value = field.defaultValue;
			field.focus();
			return false;
	}
	for (i=0; i<4 ; i++)
	{
		if (isNaN(ip_addr[i]) == true)
		{
			alert('It should be a [0-9] number.');
			field.value = field.defaultValue;
			field.focus();
			return false;
		}
	}
	if (ismask) {
		for (i=0; i<4; i++) 
		{
			if ((ip_addr[i] > 255) || (ip_addr[i] < 0))
			{
				alert('Mask IP adress format error, please keyin like 255.255.255.0.');
				field.value = field.defaultValue;
				field.focus();
				return false;
			}
		}
	}
	else {
		for (i=0; i<3; i++) 
		{
			if ((ip_addr[i] > 255) || (ip_addr[i] < 0))
			{
				alert('IP adress format error, please keyin like 10.10.10.254.');
				field.value = field.defaultValue;
				field.focus();
				return false;
			}
		}
		if ((ip_addr[i] > 254) || (ip_addr[i] < 0))
		{
				alert('IP adress format error.');
				field.value = field.defaultValue;
				field.focus();
				return false;
		}
	}

	return true;
}

var http_request = false;
function makeRequest(url, content, handler) {
	http_request = false;
	if (window.XMLHttpRequest) { // Mozilla, Safari,...
		http_request = new XMLHttpRequest();
		if (http_request.overrideMimeType) {
			http_request.overrideMimeType('text/xml');
		}
	} else if (window.ActiveXObject) { // IE
		try {
			http_request = new ActiveXObject("Msxml2.XMLHTTP");
		} catch (e) {
			try {
			http_request = new ActiveXObject("Microsoft.XMLHTTP");
			} catch (e) {}
		}
	}
	if (!http_request) {
		alert('Giving up :( Cannot create an XMLHTTP instance');
		return false;
	}
	http_request.onreadystatechange = handler;
	http_request.open('POST', url, true);
	http_request.send(content);
}

function makeRequest2(url, content, handler, args) {
	var http_request2 = false;
	if (window.XMLHttpRequest) { // Mozilla, Safari,...
		http_request2 = new XMLHttpRequest();
		if (http_request2.overrideMimeType) {
			http_request2.overrideMimeType('text/xml');
		}
	} else if (window.ActiveXObject) { // IE
		try {
			http_request2 = new ActiveXObject("Msxml2.XMLHTTP");
		} catch (e) {
			try {
				http_request2 = new ActiveXObject("Microsoft.XMLHTTP");
			} catch (e) {}
		}
	}
	if (http_request2 == null) {
		alert('Giving up :( Cannot create an XMLHTTP instance');
		return false;
	}
	http_request2.onreadystatechange = function(){
		if (http_request2.readyState == 4) {
			if (http_request2.status == 200 || http_request2.status == 304) {
				handler(http_request2, args);
			} else {
				//alert("Call " + url + " failed!!");
			}
		}
	}

	http_request2.open('POST', url, true);
	http_request2.send(content);
}

function submit_reload()
{
	setTimeout("window.location.reload()", 100);
}

function cooldown_time(cool_sec)
{
	document.getElementById("cooldown_time_text").innerHTML = cool_sec;
	if(cool_sec > 1)
		setTimeout(cooldown_time, 1000, cool_sec - 1);
}

function show_waiting(mode)
{
	if(mode == "ON"){
		display("table", "div_waiting_img");
	} else {
		display("none", "div_waiting_img");
	}
}

function disabled_buttons(buttons)
{
	var len = 0;
	var i = 0;

	if(!Array.isArray(buttons))
		return;

	len = buttons.length;
	for(i = 0; i < len; i++){
		document.getElementById(buttons[i]).disabled = true;
	}
}

function submit_form(this_button, reload_sec, buttons,  pre_check_fun)
{
	//console.log("pre_check_fun=["+ pre_check_fun +"]");
	if(pre_check_fun){
		//console.log("pre_check_fun!=NULL");
		if(!pre_check_fun()){
			//console.log("after pre_check_fun");
			return;
		}
	}else{
		//console.log("pre_check_fun==NULL");
	}

	this_button.disabled = true;
	show_waiting("ON");
	disabled_buttons(buttons);
	setTimeout("window.location.reload()", reload_sec * 1000);
	cooldown_time(reload_sec);

	this_button.form.submit();
}

