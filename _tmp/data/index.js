var
	BASE = '',
	// sensor types
	SE_VERSION = 0,
	SE_TEMPERATURE = 1,
	SE_WATER_PUMP = 2,
	SE_TWO_STATE_TAP = 4,
	SE_GAS_FURNACE = 5,
	SE_THERMOSTAT = 6,
	// device types
	DEV_SYSTEM = 103,
	stateLoader = null, // state load timer
	stateLoadReq = null, // state load request
	temperatureInfo =
		"<div class='row py-2'>" +
			"<div class='col-8 obj-name'>{name}</div>" +
			"<div class='col-3 attr'>{t}</div>" +
			"<div class='col-1'>&nbsp;</div>" +
//			"<div class='col attr'>{tMin}</div>" +
//			"<div class='col attr'>{tMax}</div>" +
		"</div>",
	gateInfo =
		"<div class='row py-2 {state}'>" +
			"<div class='col-8 obj-name'>{name}</div>" +
			"<div class='col-3 on text-right'>{onState}</div>" +
			"<div class='col-3 off text-right'>{offState}</div>" +
			"<div class='col-1'>&nbsp;</div>" +
		"</div>",
  tpl = {
  }, a, s;

function fillTemperatureTpl(data) {
	return temperatureInfo.replace(/{name}|{t}|{tMin}|{tMax}/g, function (s) {
		switch (s) {
			case '{name}': return data.name;
			case '{t}': return data.t;
			case '{tMin}': return data.tMin;
			case '{tMax}': return data.tMax;
		}
		return '';
	});
}

function fillGateTpl(data) {
	return gateInfo.replace(/{name}|{state}|{onState}|{offState}/g, function (s) {
		switch (s) {
			case '{name}': return data.name;
			case '{state}': return data.isOpen ? "on" : "off";
			case '{onState}': return data.onState === "" ? "" : "<i class='fa fa-" + data.onState + "'></i>";
			case '{offState}': return data.offState === "" ? "" : "<i class='fa fa-" + data.offState + "'></i>";
		}
		return '';
	});
}

function appendTemperature(data) {
	$("#temperatureContent").append(fillTemperatureTpl(data));
}

function appendGate(data) {
	$("#gateContent").append(fillGateTpl(data));
}

function showErrorLevel(l) {
	var c = "darkseagreen";
	switch (l) {
		case 1:
			c = "gold";
			break;
		case 2:
			c = "coral";
			break;
	}
	$("#systemCont").css("backgroundColor", c);
}

function restartStateLoader(loadNow) {
	if (stateLoader) {
		// abort a running state load request
		if (stateLoadReq) stateLoadReq.abort();
		clearInterval(stateLoader);
	}
	if (loadNow) getState();
	// reset the loading interval
	stateLoader = setInterval(getState, 5 * 1000);
}

function getState() {
	stateLoadReq = $.ajax({
		url: BASE + "/getState",
		type: "GET",
		success: function(ret) {
			$("#temperatureContent").empty();
			$("#gateContent").empty();
			var a = JSON.parse(ret), dt, s;
			a.forEach(function(el) {
				switch (el.type) {
					case SE_VERSION:
						$("#version").html(el.version);
						break;
					case SE_TEMPERATURE:
						appendTemperature(el);
						break;
					case SE_WATER_PUMP:
						el.onState = "refresh";
						el.offState = "";
						appendGate(el);
						break;
					case SE_TWO_STATE_TAP:
						el.onState = "level-up";
						el.offState = "level-down";
						appendGate(el);
						break;
					case SE_GAS_FURNACE:
						el.onState = "fire";
						el.offState = "";
						appendGate(el);
						break;
					case SE_THERMOSTAT:
						el.onState = "home";
						el.offState = "";
						appendGate(el);
						break;
					case DEV_SYSTEM:
						s = el.state + ": " + el.stateName;
						if (el.comment !== undefined && el.comment !== "") s += " [" + el.comment + "]";
						$("#systemState").html(s);
						showErrorLevel(el.errorLevel);
						break;
				}
			});
			dt = new Date();
			$("#last-update").html(dt.toLocaleTimeString("en-US", { hour12: false }));
		}
	}).always(function() {
		stateLoadReq = null;
	});
}

$(function() {
	restartStateLoader(true);
});
