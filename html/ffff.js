status = null;

function viztitlegen(titlestr) {
	var title = document.getElementById("title");
	title.innerHTML = "<h2>"+titlestr+"</h2>";
	title.innerHTML += "<br><a href=index.html>Vizlets Control</a>";
	title.innerHTML += "<br><a href=ffff.html>FFFF Control</a>";
	title.innerHTML += "<br><a href=vizserver.html>VizServer Control</a>";
	title.innerHTML += "<br><a href=params.html>Params Editor</a>";
}

function changeval(name) {
	var valueid = document.getElementById("value_"+name);
	var rangeid = document.getElementById("range_"+name);
	valueid.value = rangeid.value;
}

function sendparamchange(name) {
	var valueid = document.getElementById("value_"+name);
	var a = name.split(".");
	var instance = a[0];
	var paramname = a[1];
	var paramval = valueid.value;
	var api = vizapi("ffff.ffglparamset","{\"instance\":\""+instance+"\", \"param\":\""+paramname+"\", \"val\":\""+paramval+"\" }");
}

function updateval(name) {
	changeval(name);
	sendparamchange(name);
}

function changeenable(instance) {
	var enabledid = document.getElementById("enabled_"+instance);
	var api;
	if ( enabledid.checked ) {
		api = "ffff.ffglenable";
	} else {
		api = "ffff.ffgldisable";
	}
	var a = vizapi(api,"{\"instance\":\""+instance+"\" }");
}

// updateval2 propagate changes from value_ to range_ rather
// than range_ to value_
function updateval2(name) {
	var valueid = document.getElementById("value_"+name);
	var rangeid = document.getElementById("range_"+name);
	rangeid.value = valueid.value;
	sendparamchange(name);
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

function loadpipeline() {
	var fname = document.getElementById("pipelinefilename").value;
	var api = vizapi("ffff.loadpipeline","{\"filename\":\""+fname+"\"}");
	var status = document.getElementById("status");
	if ( api.error != "" ) {
		status.innerHTML = api.error;
	} else {
		status.innerHTML = "OK";
	}
}

function savepipeline() {
	var fname = document.getElementById("pipelinefilename").value;
	var api = vizapi("ffff.savepipeline","{\"filename\":\""+fname+"\"}");
	if ( api.error != "" ) {
		status.innerHTML = api.error;
	} else {
		status.innerHTML = "OK - pipeline saved";
	}
}
function ffffpagegen() {

	var api = vizapi("ffff.ffglpipeline");
	if ( api.error != "" ) {
		status.innerHTML = api.error;
		return;
	}

	ffglpipeline = api.result;

	var html = "";

	html += "<hr>\n";
	html += "<table>";
	html += "<tr><td>Status:</td><td><div id=\"status\">OK</div></td></tr>";
	html += "<tr><td>";
	html += "<input type=\"file\" style=\"display: none\" id=\"pipelinefile\" onChange=\"changepipelinefile();\" >";
	html += "Pipeline File: ";
	html += "</td><td>";
	html += "<input type=\"text\" style=\"width: 100%\" id=\"pipelinefilename\">";
	html += "</td>";
	html += "</table>";

	html += "<p>";
	html += "<input type=\"button\" style=\"width:100px\" value=\"Browse\" onClick=\"browsepipelinefile();\">";
	html += "&nbsp;&nbsp;";
	html += "<input type=\"button\" style=\"width:100px\" value=\"Load\" onClick=\"loadpipeline();\">";
	html += "&nbsp;&nbsp;";
	html += "<input type=\"button\" style=\"width:100px\" value=\"Save\" onClick=\"savepipeline();\">";

	var r;
	for ( var i=0; i<ffglpipeline.length; i++ ) {

		var f = ffglpipeline[i];

		html += "<p><hr>\n";
		html += "<table>";

		var enabledid = "enabled_"+f.instance;
		var checked = "";
		if ( f.enabled ) {
			checked = "checked=\"checked\"";
		}

		html += "<tr><td><b>"+f.instance+"</b>";
		html += "<input type=\"checkbox\" "+checked+" id=\""+enabledid+"\" onchange=\"changeenable('"+f.instance+"')\")>";
		html += "</td>";
		html += "</tr>";

		var api = vizapi("ffff.ffglparamvals","{\"instance\":\""+f.instance+"\"}");
		if ( api.error != "" ) {
			alert("Error getting ffglparamvals: "+api.error);
			continue;
		}
		var vals = api.result;

		for ( var n=0; n<vals.length; n++ ) {
			var v = vals[n];
			// html += "<p>"+v.name+" "+v.value+"\n";
			html += "<tr><td width=15%></td>";

			var name = f.instance+"."+v.name;
			var valueid = "value_"+name;
			var rangeid = "range_"+name;

			var argstr = "&quot;instance&quot; : "
				+ "&quot;'+document.getElementById(&quot;" + valueid + "&quot;).value+'&quot;"
				;

			// html += "<td><input type=\"button\" style=\"width:100px\" value=\""+v.name+"\" onClick=\"vizapi('ffff.ffglparamset','{"+argstr+"}');\"></td>";
			html += "<td align=right width=25%>"+v.name+"</td>";

			var mn = 0.0;
			var mx = 1.0;
			var stepsize = 0.01;

			html += "<td width=10></td><td width=25%><input style=\"width:98%\" value=\""+v.value+"\" type=\"number\" min=\""+mn+"\" max=\""+mx+"\" id=\""+valueid+"\" size=8 onchange=\"updateval2('"+name+"');\" ></td>";

			html += "<td><input style=\"width:90%\" id=\""+rangeid+"\" type=\"range\" min=\""+mn+"\" max=\""+mx+"\" step=\""+stepsize+"\" oninput=\"changeval('"+name+"');\" onchange=\"updateval('"+name+"');\" ></td>";

			html += "</tr>";
		}
		html += "</table>";
	}
	html += "<br><hr>\n";

	document.getElementById("ffglpipeline").innerHTML = html;
	status = document.getElementById("status");
}
