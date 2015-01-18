function format_error(meth,err) {
	var estr = "" + err;
	if ( estr.match(/xmlhttprequest exception 101/i) ) {
		return "Unable to connect to MMTT - is it running?";
	} else {
		return "Method: "+meth+"  Error:" + err;
	}
}

function doapi(meth,params) {

	// The value of params should include the curly braces
	params = (typeof params === "undefined") ? "{ }" : params;

	var id = "12345";
	var url = "/api";

	var postData = "{ \"jsonrpc\": \"2.0\", \"method\": \""+meth+"\", \"params\": "+params+", \"id\":\""+id+"\" }\n";

	var def = dojo.xhrPost( {
		url: url,
		handleAs: "json",
		headers: { "Content-Type": "application/json"},
		sync: true,
		postData: "{ \"jsonrpc\": \"2.0\", \"method\": \""+meth+"\", \"params\": "+params+", \"id\":\""+id+"\" }\n"
	});

	var retval = "";
	var errval = "";

	def.then( function(r) {
		if ( r == null ) {
			errval = "Method: "+meth+"  Error! r is null";
		} else if ( r.error ) {
			errval = "Method: "+meth+"  Error! id:" + r.id + " msg="+r.error.message;
		} else {
			retval = r.result;
		}
		},
		function(err) {
			errval = format_error(meth,err);
		}
	);

	return {result: retval, error: errval};
}
