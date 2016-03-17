status = null;

function changeval(name) {
	var valueid = document.getElementById("value_"+name);
	var rangeid = document.getElementById("range_"+name);
	valueid.value = rangeid.value;
}

function sendparamchange(name) {
	var valueid = document.getElementById("value_"+name);
	var a = name.split(".");
	var viztag = a[0];
	var paramname = a[1];
	var paramval = valueid.value;
	var api = vizapi("ffff.ffglparamset","{\"viztag\":\""+viztag+"\", \"param\":\""+paramname+"\", \"val\":\""+paramval+"\" }");
	checkapi(api);
}

function updateval(name) {
	changeval(name);
	sendparamchange(name);
}

function changeenable(viztag) {
	var enabledid = document.getElementById("enabled_"+viztag);
	var onoff = enabledid.checked ? "1" : "0";
	var a = vizapi("ffff.ffglenable","{\"viztag\":\""+viztag+"\", \"onoff\": "+onoff+" }");
	checkapi(api);
}

function changemoveable(viztag) {
	var moveableid = document.getElementById("moveable_"+viztag);
	var onoff = moveableid.checked ? "1" : "0";
	var a = vizapi("ffff.ffglmoveable","{\"viztag\":\""+viztag+"\", \"onoff\": "+onoff+" }");
	checkapi(api);
}

// updateval2 propagate changes from value_ to range_ rather
// than range_ to value_
function updateval2(name) {
	var valueid = document.getElementById("value_"+name);
	var rangeid = document.getElementById("range_"+name);
	rangeid.value = valueid.value;
	sendparamchange(name);
}

function moveup(viztag) {
	var api = vizapi("ffff.moveup","{\"viztag\":\""+viztag+"\"}");
	checkapi(api);
	ffffpagegen();
}

function movedown(viztag) {
	var api = vizapi("ffff.movedown","{\"viztag\":\""+viztag+"\"}");
	checkapi(api);
	ffffpagegen();
}

function browsepipelinefile() {
	var pipelinefile = document.getElementById("pipelinefile");
	pipelinefile.click();
}

function changepipelinefile() {
	var pipelinefilename = document.getElementById("pipelinefilename");
	var pipelinefile = document.getElementById("pipelinefile");
	var val = pipelinefile.value;
	var words = val.split(/[\\\/]/);
	if ( words.length > 1 ) {
		val = words[words.length-1];
	}
	pipelinefilename.value = val;
	loadpipeline();
}

function refreshpipelinefilename() {
	var api = vizapi("ffff.pipelinefilename");
	var pipelinefilename = "";
	if ( checkapi(api) ) {
		pipelinefilename = api.result;
	}
	if ( pipelinefilename != "" ) {
		document.getElementById("pipelinefilename").value = pipelinefilename;
	}
}

function loadpipeline() {
	var fname = document.getElementById("pipelinefilename").value;
	var api = vizapi("ffff.loadpipeline","{\"filename\":\""+fname+"\"}");
	status = document.getElementById("status");
	checkapi(api);
	ffffpagegen();
	// refreshpipelinefilename();
}

function savepipeline() {
	var fname = document.getElementById("pipelinefilename").value;
	var api = vizapi("ffff.savepipeline","{\"filename\":\""+fname+"\"}");
	checkapi(api,"Pipeline saved!");
}

function shufflepipeline() {
	var api = vizapi("ffff.shufflepipeline");
	if ( checkapi(api) ) {
		ffffpagegen();
	}
}

function randomizepipeline() {
	var api = vizapi("ffff.randomizepipeline");
	if ( checkapi(api) ) {
		ffffpagegen();
	}
}

function ffffrecord(onoff) {
	var api = vizapi("ffff.record","{\"onoff\": "+onoff+" }");
	checkapi(api);
}

function enableall(onoff) {
	var api = vizapi("ffff.ffglpipeline");
	if ( ! checkapi(api) ) {
		return;
	}
	var ffglpipeline = api.result;
	for ( var i=0; i<ffglpipeline.length; i++ ) {

		var f = ffglpipeline[i];
		var a = vizapi("ffff.ffglenable","{\"viztag\":\""+f.viztag+"\", \"onoff\": "+onoff+" }");
	}
	ffffpagegen();
}

function changeTextVal(tag,api,argname) {
	var inputid = document.getElementById(tag+"_"+api);
	var api = vizapi(tag+"."+api,"{\""+argname+"\":\""+inputid.value+"\"}");
	if ( ! checkapi(api) ) {
		inputid.value = "";
		return;
	}
}

function params_class_of_api(api) {
	var p;
	if ( api.search("midi") >= 0 ) {
		p = "midi";
	} else if ( api.search("sprite") >= 0 ) {
		p = "sprite";
	} else {
		p = "UNKNOWN";
	}
	return p;
}

