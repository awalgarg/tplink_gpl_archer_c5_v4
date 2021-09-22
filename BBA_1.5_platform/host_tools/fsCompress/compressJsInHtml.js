
execSync=require('child_process').execSync;
util=require('util');
fs=require('fs');
path=require('path');

/*	get the first argument as webdir	*/
var options = process.argv.splice(2);
var utilsDir = options[0];
var htmlFile = options[1];

if (path.basename(htmlFile).match('\.htm'))
{
	var str = fs.readFileSync(htmlFile,'utf8');
	var re = /(<script[^>]*?type[^>]*?text\/javascript[^>]*?>)([\w\W]*?)(<\/script>)/i;
	var re_g = /(<script[^>]*?type[^>]*?text\/javascript[^>]*?>)([\w\W]*?)(<\/script>)/gi;
	str =  str.replace(/<!--[\w\W]*?-->/g,'');
	

	var res;
	var i=0;
	var strOut = '';
	var index = 0;

	while(res = re_g.exec(str))
	{
		var match = res[0].match(re);
		var jsPrefix = match[1];
		var jsSuffix = match[3];
		var jsBlock = match[2];

		if (!jsPrefix.match(/src/) && (jsBlock.length != 0))
		{
			var jsBlockName = htmlFile+'.'+i+'.js';
			var jsBlockNameTmp = htmlFile+'.'+i+'.js.temp';

			i++;

			fs.writeFileSync(jsBlockName, jsBlock,'utf8');
			execSync("java -jar " + utilsDir + "/yuicompressor.jar --nomunge -o " + jsBlockNameTmp + ' '+jsBlockName);
			execSync('mv '+ jsBlockNameTmp + ' ' + jsBlockName);

			jsBlock = fs.readFileSync(jsBlockName);
			execSync('rm ' + jsBlockName);
		}
		
		strOut +=str.slice(index, res.index);
		strOut +=(jsPrefix + jsBlock+ jsSuffix);
		index = re_g.lastIndex;

	}

	strOut +=str.slice(index);
	//fs.writeFileSync(htmlFile + '.min.htm', strOut, 'utf8');
	fs.writeFileSync(htmlFile, strOut, 'utf8');
}