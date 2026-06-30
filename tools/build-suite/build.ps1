param(
	[Parameter(Mandatory=$true)]
	[string]$SHELLSCRIPT,
	[switch]$HELP
)

$TOOLSDIR = Join-Path (Get-Location) 'tools/'

$PE2EXEC = (Join-Path $TOOLSDIR '/executable-format/exechandler.exe')
$FSCONTROLLEREXEPATH = (Join-Path $TOOLSDIR '/filesystem/fs.exe')
$FSCONTROLLER = New-Object System.Diagnostics.Process
$FSOUTEVENT = $null
$FSERREVENT = $null
$CUSTOMGCC = Join-Path $TOOLSDIR 'build-suite/gcc.ps1'

function Open-Log{
	param([string]$LOGFILE)
	if($script:BuildFeatures.LOG){
		if(Test-Path $LOGFILE){$script:BuildFeatures.LOGFILE = $LOGFILE}
		elseif(Test-Path (Join-Path (Get-Item $script:BuildFeatures.LOGFILE).Parent.FullName ($LOGFILE -replace ".",""))){
			$script:BuildFeatures.LOGFILE = $LOGFILE
		}
		if($script:BuildFeatures.LOGFILE){Set-Content -Path $script:BuildFeatures.LOGFILE -Value $null}
	}
}
function global:Write-Log{
	param([string]$MSG, [System.ConsoleColor]$COLOR)
	if($script:BuildFeatures.LOGFILE){
		Add-Content -Path $script:BuildFeatures.LOGFILE -Value $MSG
	}
	if($COLOR){Write-Host $MSG -ForegroundColor $COLOR}
	else{Write-Host $MSG}
}

function Show-Help{
	param()
	Write-Log @"
	This build system essentially just manages the creation of BlockyOS-Compatible Executables && their passing to the FrAT FileSystem.
	It will use the utilities in the tools Directory to manage the whole Build System.
	It will work by parsing a script.
	The Script additionally supports Enables; Using the Syntax: #<FEATURE>:<ENABLED/DISABLED>; 
		wherein the Behaviour must be enabled before being syntatically enabled, 
		Enables can be DISABLED and ENABLED at any point in the Duration of the Script.
		
		The Following Enables are supported:
			#JSON			Enable the Json Header
			#CACHE			Enable Caching of CGCC Files, <THIS FEATURE ISN'T FULLY INITIALISED>
			#DEBUG			Enable Debugging Output
			#FSFRAT.RUNTIME	Toggle running FrAT Executable as a Static Service or a RunTime.
	The script syntax starts with a .json header: <REQUIRING THE ENABLE: "#JSON:ENABLED">
	{
		"LOG": PATH,
		"SCRIPT.VERSION": DIGIT,
		"FS.IMAGE": IMAGE-PATH,
		"FS.FORMAT": {
			"TARGET.PARTITION": NAME,
			"TARGET.BLOCK.SIZE": NUMBER,
			"TARGET.LOG.BLOCKS": NUMBER,
			"TARGET.VERSION": MAJOR:MINOR
		},
		"INPUT": {
			"EXECUTABLE": [
				{
					"OUTPUT": PATH,
					"CARGS": "CARGS",
					"LARGS": "LARGS",
					"FILES": {
						...(Files in various Compilation stages)
					}
				}, "EXECUTABLE": {
					"OUTPUT": PATH,
					"CARGS": "CARGS",
					"LARGS": "LARGS",
					"FILES": {
						...(Files in various Compilation stages)
					}
				}, "EXECUTABLE": {
					"OUTPUT": PATH,
					"CARGS": "CARGS",
					"LARGS": "LARGS",
					"FILES": {
						...(Files in various Compilation stages)
					}
				}, ..., ..., ...
			]
		}
	}

	Additionaly there will be a suite of commands to use
	SHELL has the Command(s):
		.REBUILD
	CGCC has the command(s):
		.MAKE 
			-f @(<VARIOUS FILE PATHS>)
			-o <OUTPUT PATH>
			-c @(<VARIOUS COMPILE ARGUMENTS>)
			-l @(<VARIOUS COMPILE ARGUMENTS>)
	PE2EXEC has the command(s):
		.MAKE
			-d <INT>
			-i @(<VARIOUS FILE PATHS>)
			-o <OUTPUT PATH>
	FSFRAT has the command(s):
		.MOUNT
			-p 	<PATH TO IMAGE>
			-pt <PARTITION-NAME>
			-bs <BLOCK SIZE>
			-lb <N LOG BLOCKS>
			-v <VERSION "MAJOR:MINOR">
		.ALIAS
			-% <ALIAS> 
			-d <DATA>
		.OPEN
			-p <PATH> 
			-a <NAME> 
			-la <ARGS>
		.CLOSE
			-a <NAME>
		.DELETE
			-p <NAME>
		.MAKE
			-p <FILE-PATH> 
			-n <NAME> 
			-la <ARGS>
		.READ
			-pp <NAME> 
			-ip <OUTPUT-FILE-PATH>
			-p <INT>
			-n <NBYTES>
		.WRITE
			-pp <NAME> 
			-ip <INPUT-FILE-PATH>
			-p <INT>
			-n <NBYTES>
"@
}

# Global Registries
# Feature registry: add keys here to allow directives
$script:BuildFeatures = @{
	JSON  = $false
	CACHE = $false
	DEBUG = $false
	'FSFRAT.RUNTIME' = $true
}

# Command registry: maps command name -> @{ MinArgs = n; MaxArgs = m }
$script:CommandRegistry = @{}
$script:FSSET = $false
# Strict schema definition as nested hashtable
$script:JsonSchema = @{
	"LOG" = "string"
	"SCRIPT.VERSION" = "int"
	"FS.IMAGE" = "string"
	"FS.FORMAT" = @{
		"TARGET.PARTITION" = "string"
		"TARGET.BLOCK.SIZE" = "int"
		"TARGET.LOG.BLOCKS" = "int"
		"TARGET.VERSION" = "string"
	}
	"INPUT?" = @{
		# Handle up to 
		"EXECUTABLE **0, 1024**" = @{
			"OUTPUT" = "string"
			"CARGS"  = "string"
			"LARGS"  = "string"
			"FILES"  = "object"
		}
	}
}

function Register-Feature{
	param([string]$Name, [bool]$Default = $false)
	$script:BuildFeatures[$Name]= $Default
}

function Register-Command{
	param(
		[string]$Name,
		[int]$MinArgs,
		[int]$MaxArgs,
		[hashtable]$FlagSpec
	)

	if(($MinArgs -lt 0) -or ($MaxArgs -lt $MinArgs)){throw "[RegisterError]Invalid arity for command $Name"}
	$script:CommandRegistry[$Name] = @{
		MinArgs  = $MinArgs
		MaxArgs  = $MaxArgs
		FlagSpec = $FlagSpec   # <— Correct key name
	}
}

# Token and AST Classes
class BuildToken{
	[string]$Type
	[string]$Value
	[int]$Line
	[int]$Column
	BuildToken([string]$t, [string]$v, [int]$l, [int]$c){
		$this.Type = $t; $this.Value = $v; $this.Line = $l; $this.Column = $c
	}
	[string]ToString(){ return "$($this.Type) '$($this.Value)' @ $($this.Line):$($this.Column)" }
}

class AstBuildScript{
	[hashtable]$Header
	[System.Collections.Generic.List[AstCommand]]$Commands
	AstBuildScript(){ $this.Commands = [System.Collections.Generic.List[AstCommand]]::new() }
}

class AstCommand{
	[string]$Name
	[hashtable]$Flags
	[System.Collections.Generic.List[psobject]]$Args
	[int]$Line

	AstCommand([string]$name, [int]$line){
		$this.Name  = $name
		$this.Line  = $line
		$this.Flags = @{}   # now stores flag → @{Type=...; Value=...}
		$this.Args  = [System.Collections.Generic.List[psobject]]::new()
	}
}

# Error helpers
function Throw-ParseError{
	param([string]$Message, [BuildToken]$Token)
	if($Token){
		throw "[ParseError]$Message (at line $($Token.Line), col $($Token.Column), token '$($Token.Value)')"
	}else{throw "[ParseError]$Message"}
}

function Throw-HeaderError{
	param([string]$Message)
	throw "[HeaderError]$Message"
}

function Throw-SchemaError{
	param([string]$Message)
	throw "[SchemaError]$Message"
}

# Directive processing
function Process-Directive{
	param([string]$Line)
	# Accept forms: #JSON:ENABLED, #CACHE:ON, #DEBUG:OFF
	if($Line -match '^\s*#\s*([A-Z0-9_]+)\s*:\s*(ENABLED|DISABLED|ON|OFF)\s*$'){
		$feature = $matches[1]
		$state = $matches[2]
		if(-not $script:BuildFeatures.ContainsKey($feature)){
			throw "[FeatureError]Unknown feature directive: $feature"
		}
		$script:BuildFeatures[$feature]= ($state -in 'ENABLED','ON')
		return $true
	}
	return $false
}

# JSON header extraction and validation
function Get-JsonHeader{
	param([string[]]$Lines)
	$jsonEnabled = $false
	$inJson = $false
	$braceDepth = 0
	$jsonLines = @()
	$endIndex = -1

	for($i = 0; $i -lt $Lines.Count; $i++){
		$line = $Lines[$i]

		# Process directives even before JSON block
		Process-Directive $line | Out-Null

		if(-not $jsonEnabled -and ($line -match '^\s*#\s*JSON:ENABLED')){
			$jsonEnabled = $true
			continue
		}

		if(-not $jsonEnabled){continue}

		$stripped = $line -replace '^\s*#\s?', ''

		if(-not $inJson){
			if($stripped -match '^\s*\{'){
				$inJson = $true
			}else{continue}
		}

		$jsonLines += $stripped
		$braceDepth += ($stripped.ToCharArray() | Where-Object { $_ -eq '{' }).Count
		$braceDepth -= ($stripped.ToCharArray() | Where-Object { $_ -eq '}' }).Count

		if($braceDepth -eq 0){
			$endIndex = $i
			break
		}
	}

	if(-not $jsonEnabled){return @{ Header = $null; EndIndex = -1 }}
	if($jsonLines.Count -eq 0){Throw-HeaderError "JSON header not found after #JSON:ENABLED"}

	$jsonText = $jsonLines -join "`n"
	try{$jsonObj = $jsonText | ConvertFrom-Json -AsHashTable -ErrorAction Stop
	}catch{Throw-HeaderError "Invalid JSON header: $($_.Exception.Message)"}

	return [hashtable]@{Header = $jsonObj; EndIndex = $endIndex}
}

function Validate-JsonSchema{
	param([hashtable]$Json, [hashtable]$Schema, [string]$Path = "")

	foreach ($rawKey in $Schema.Keys){

		$key = $rawKey
		$isOptional = $false

		# Detect optional field: NAME?
		if($key -match '^(?<name>[A-Za-z0-9\._]+)\?$'){
			$isOptional = $true
			$key = $matches['name']
		}

		# Detect multiplicity: NAME **min, max**
		$hasMultiplicity = $rawKey -match '^(?<name>[A-Za-z0-9\._]+)\s+\*\*\s*(?<min>\d+)\s*,\s*(?<max>\d+|\∞)\s*\*\*$'

		if($hasMultiplicity){

			$name = $matches['name']
			$min  = [int]$matches['min']
			$max  = $matches['max']

			$full = if($Path){"$Path.$name"}else{$name}

			# Optional multiplicity: allow missing field
			if($isOptional -and -not $Json.PSObject.Properties.Name.Contains($name)){continue}

			# Required multiplicity: must exist
			if(-not $Json.PSObject.Properties.Name.Contains($name)){
				Throw-SchemaError "Missing required JSON field: $full (requires $min to $max entries)"
			}

			$value = $Json.$name

			if(-not ($value -is [System.Collections.IEnumerable])){
				Throw-SchemaError "Field $full must be an array because schema declares multiplicity."
			}

			$count = $value.Count

			if($count -lt $min){Throw-SchemaError "Field $full requires at least $min entries, found $count."}

			if($max -ne '∞' -and $count -gt [int]$max){
				Throw-SchemaError "Field $full allows at most $max entries, found $count."
			}

			$expectedSchema = $Schema[$rawKey]
			foreach($item in $value){Validate-JsonSchema $item $expectedSchema $full}
			continue
		}

		# Normal (non-multiplicity) field
		$full = if($Path){"$Path.$key"}else{$key}

		# Optional field: allow missing
		if($isOptional -and -not $Json.PSObject.Properties.Name.Contains($key)){
			continue
		}

		# Required field: must exist
		if(-not $Json.Contains($key)){
			Throw-SchemaError "Missing required JSON field: $full"
		}

		$expected = $Schema[$rawKey]
		$value = $Json.$key

		switch($expected){
			"int" {
				if(-not (
					(($value -is [byte]) -or ($value -is [Int16]) -or ($value -is [Int32]) -or ($value -is [Int64]) -or ($value -is [System.Int128])) -or 
					(($value -is [string]) -and ($value -match '^[0-9]+$')))
				){
					Throw-SchemaError "Field $full must be integer"
				}
			} "string" {
				if(-not ($value -is [string])){Throw-SchemaError "Field $full must be string"}
			} "version" {
				if(-not (($value -is [string]) -and ($value -match '^[0-9]+:[0-9]+$'))){
					Throw-SchemaError "Field $full must be MAJOR:MINOR"
				}
			} "object" {
				if(-not ($value -is [psobject])){Throw-SchemaError "Field $full must be object"}
			} default {Validate-JsonSchema $value $expected $full}
		}
	}
	return $true
}

function Split-CommandChain{
	param([string]$Line)

	$parts = @()
	$current = ""
	$inString = $false
	$stringChar = ''
	$depth = 0
	$i = 0

	while($i -lt $Line.Length){
		$c = $Line[$i]

		# Track list literal parentheses
		if(-not $inString){
			if($c -eq '('){$depth++}
			elseif($c -eq ')'){$depth--}
		}

		# Track string state
		if($c -eq '"' -or $c -eq "'"){
			if(-not $inString){
				$inString = $true
				$stringChar = $c
			}elseif($stringChar -eq $c){$inString = $false}
		}

		# Detect && only when not in string and not in list literal
		if(-not $inString -and $depth -eq 0 -and $i -lt $Line.Length - 1 -and $c -eq '&' -and $Line[$i+1] -eq '&'){
			$parts += $current.Trim()
			$current = ""
			$i += 2
			continue
		}

		$current += $c
		$i++
	}

	if($current.Trim().Length -gt 0){$parts += $current.Trim()}
	return $parts
}

function Validate-ChainNamespaces{
	param([string[]]$Commands)
	function Get-Namespace{
		param([string]$cmd)
		if($cmd -match '^([A-Za-z0-9_]+)\.'){return $matches[1]}
		throw "Invalid command format: $cmd"
	}

	$namespaces = $Commands | ForEach-Object{Get-Namespace $_}
	$unique = $namespaces | Select-Object -Unique

	if($unique.Count -gt 1){
		throw "[ChainError] Commands chained with && must belong to the same driver. Found: $($unique -join ', ')"
	}
}


# Tokenizer
function Tokenize-Lines{
	param([string[]]$Lines, [int]$StartLine = 1)

	$tokens = [System.Collections.Generic.List[BuildToken]]::new()
	for($i = 0; $i -lt $Lines.Count; $i++){
		$unprocessedLineText = $Lines[$i]
		$lineNum = $StartLine + $i

		# If directive line, skip tokenizing it; directive already processed
		$splittext = Split-CommandChain $unprocessedLineText
		if(($unprocessedLineText -match '^\s*#') -or (-not $splittext)){continue}
		(Validate-ChainNamespaces $splittext)
		foreach($lineText in $splittext){
			$cursor = 0
			while($cursor -lt $lineText.Length){
				$rest = $lineText.Substring($cursor)
				$col = $cursor + 1
	
				if($rest -match '^\s+'){ $cursor += $matches[0].Length; continue }
	
				# List literal @( ... ) capture balanced parentheses
				if($rest -match '^@\('){
					$depth = 1; $j = 2
					while($j -lt $rest.Length -and $depth -gt 0){
						if($rest[$j]-eq '('){ $depth++ }
						elseif($rest[$j]-eq ')'){ $depth-- }
						$j++
					}
					if($depth -ne 0){ Throw-ParseError "Unclosed list literal '@('." ([BuildToken]::new('ListLiteral',$rest,$lineNum,$col)) }
					$value = $rest.Substring(0,$j)
					$tokens.Add([BuildToken]::new('ListLiteral',$value,$lineNum,$col))
					$cursor += $j
					continue
				}
	
				# Flag literal: -flag or --flag
				if($rest -match '^-{1,2}[A-Za-z0-9\.\-_]+'){
					$tokens.Add([BuildToken]::new('Flag',$matches[0],$lineNum,$col))
					$cursor += $matches[0].Length
					continue
				}
	
				# String literal "..."
				if($rest -match '^"([^"\\]*(\\.[^"\\]*)*)"'){
					$tokens.Add([BuildToken]::new('String',$matches[1],$lineNum,$col))
					$cursor += $matches[0].Length
					continue
				}
	
				# Identifier (allow path chars)
				if($rest -match '^[A-Za-z0-9\.\-_:/\\]+'){
					$tokens.Add([BuildToken]::new('Identifier',$matches[0],$lineNum,$col))
					$cursor += $matches[0].Length
					continue
				}
				
	
				# Unknown single char
				$tokens.Add([BuildToken]::new('Symbol',$rest[0],$lineNum,$col))
				$cursor++
			}
			# End-of-line token
			$tokens.Add([BuildToken]::new('EOL','\n',$lineNum,$lineText.Length + 1))
		}
	}


	# EOF
	$tokens.Add([BuildToken]::new('EOF','',$StartLine + $Lines.Count,1))
	return $tokens
}

# Parser (recursive-descent)
class BuildParser{
	[System.Collections.Generic.List[BuildToken]]$Tokens
	[int]$Index

	BuildParser([System.Collections.Generic.List[BuildToken]]$tokens){
		$this.Tokens = $tokens; $this.Index = 0
	}

	[BuildToken]Current(){ return $this.Tokens[$this.Index]}
	[BuildToken]Advance(){ $t = $this.Current(); $this.Index++; return $t }
	[bool]Match([string]$type){
		if($this.Current().Type -eq $type){ $this.Advance() | Out-Null; return $true }
		return $false
	}
	[BuildToken]Expect([string]$type, [string]$message){
		if($this.Current().Type -ne $type){ Throw-ParseError $message $this.Current() }
		return $this.Advance()
	}

	[string]GetValueType($arg){
		switch($arg.NodeType){
			"String"     { return "string" }
			"Number"     { return "int" }
			"List"       { return "list" }
			"Identifier" { return "identifier" }
			default      { return "unknown" }
		}
		return "unknown"
	}

	[AstBuildScript]ParseScript(){
		$script = [AstBuildScript]::new()
		while($this.Current().Type -ne 'EOF'){
			if($this.Match('EOL')){ continue }
			$cmd = $this.ParseCommand()
			if($cmd){
				# Validate command against registry
				Validate-Command $cmd
				$script.Commands.Add($cmd)
			}
		}
		return $script
	}

	[AstCommand]ParseCommand(){
		$nameTok = $this.Current()
		if($nameTok.Type -ne 'Identifier'){Throw-ParseError "Expected command name (Identifier)." $nameTok}
		$this.Advance() | Out-Null

		$arg = $null

		$cmd = [AstCommand]::new($nameTok.Value, $nameTok.Line)
		$pendingFlag = $null

		while($this.Current().Type -ne 'EOL' -and $this.Current().Type -ne 'EOF'){
			$tok = $this.Current()
			switch($tok.Type){
				'Flag' {
					$pendingFlag = $tok.Value
					$this.Advance() | Out-Null
					continue
				} 'String' {
					$arg = $this.ParseArgument()
					if($pendingFlag){
						$cmd.Flags[$pendingFlag] = $arg.Value
						$pendingFlag = $null
					}else{$cmd.Args.Add($arg)}
					continue
				} 'Identifier' {
					$arg = $this.ParseArgument()
					if($pendingFlag){
						$cmd.Flags[$pendingFlag] = $arg.Value
						$pendingFlag = $null
					}else{$cmd.Args.Add($arg)}
					continue
				} 'ListLiteral' {
					$arg = $this.ParseListLiteral()
					if($pendingFlag){
						$cmd.Flags[$pendingFlag] = $arg.Value
						$pendingFlag = $null
					}else{$cmd.Args.Add($arg)}
					continue
				} default {Throw-ParseError "Unexpected token in argument list: $($tok.Type)." $tok}
			}
		}

		if($pendingFlag){
			$cmd.Flags[$pendingFlag] = @{
				Type  = $this.GetValueType($arg)
				Value = $arg.Value
			}
			$pendingFlag = $null
		}else{$cmd.Args.Add($arg)}


		if($this.Match('EOL')){ }

		return $cmd
	}

	[psobject]ParseArgument(){
		$tok = $this.Current()
		switch($tok.Type){
			'String' {
				$this.Advance() | Out-Null
				return [pscustomobject]@{ NodeType='String'; Value=$tok.Value; Line=$tok.Line; Column=$tok.Column }
			}
			'Identifier' {
				$this.Advance() | Out-Null
				if($tok.Value -match '^[0-9]+$'){
					return [pscustomobject]@{ NodeType='Number'; Value=[int]$tok.Value; Line=$tok.Line; Column=$tok.Column }
				}
				return [pscustomobject]@{ NodeType='Identifier'; Value=$tok.Value; Line=$tok.Line; Column=$tok.Column }
			}
			'ListLiteral' {return $this.ParseListLiteral()}
			default {
				Throw-ParseError "Unexpected token in argument list: $($tok.Type)." $tok
				return $null
			}
		}
		return $null
	}

	[psobject]ParseListLiteral(){
		$tok = $this.Expect('ListLiteral', "Expected list literal '@(...)'.")
		$value = $null
		try{$value = Invoke-Expression $tok.Value
		}catch{Throw-ParseError "Invalid list literal syntax: $($tok.Value)" $tok}
		if(-not ($value -is [System.Collections.IEnumerable])){
			Throw-ParseError "List literal did not evaluate to a list." $tok
		}
		return [pscustomobject]@{ NodeType='List'; Value=@($value); Line=$tok.Line; Column=$tok.Column }
	}
}

# Command validation
function Validate-Command{
	param([AstCommand]$CmdNode)

	if(-not $script:CommandRegistry.ContainsKey($CmdNode.Name)){
		throw "[CommandError]Unknown command: $($CmdNode.Name) at line $($CmdNode.Line)"
	}
	$rule = $script:CommandRegistry[$CmdNode.Name]

	# Validate positional args
	$argc = $CmdNode.Flags.Count
	if($argc -lt $rule.MinArgs -or $argc -gt $rule.MaxArgs){
		throw "[CommandError]Command $($CmdNode.Name) expects $($rule.MinArgs)-$($rule.MaxArgs) args, got $argc at line $($CmdNode.Line)"
	}

	# Validate flags
	foreach($flag in $rule.FlagSpec.Keys){
		$spec = $rule.FlagSpec[$flag]

		# Required flag missing
		if($spec.Required -and -not $CmdNode.Flags.ContainsKey($flag)){
			throw "[CommandError]Missing required flag '$flag' for command $($CmdNode.Name) at line $($CmdNode.Line)"
		}

		# Type mismatch
		if($CmdNode.Flags.ContainsKey($flag)){
			$actualType   = $CmdNode.Flags[$flag].GetType().ToString()
			$expectedType = "system.$($spec.Type)"

			if(($actualType.ToLower() -replace "\d",'') -ne $expectedType.ToLower()){
				throw "[CommandError]Flag '$flag' expects type '$expectedType' but got '$actualType' at line $($CmdNode.Line)"
			}
		}
	}
	return $true
}

# Top-level parse function
function Parse-BuildFile{
	param([Parameter(Mandatory)][string]$Path)

	if(-not (Test-Path $Path)){throw "File not found: $Path"}
	$lines = Get-Content -LiteralPath $Path

	# First pass: process directives and extract header if JSON feature enabled
	$headerInfo = Get-JsonHeader -Lines $lines
	$header = $headerInfo.Header
	$headerEnd = $headerInfo.EndIndex

	# If JSON feature is enabled, validate header strictly
	if($script:BuildFeatures.JSON){
		if(-not $header){Throw-HeaderError "JSON feature enabled but header not found."}
		Validate-JsonSchema $header $script:JsonSchema | Out-Null
	}else{
		# If header exists but JSON feature disabled, error
		if($header){Throw-HeaderError "JSON header present but JSON feature is disabled. Enable with #JSON:ENABLED"}
	}

	# Prepare command lines (lines after header)
	$commandLines = @()
	if($headerEnd -ge 0 -and $headerEnd -lt $lines.Count - 1){
		$commandLines = @()
		$rawLines = if($headerEnd -ge 0){
			$lines[($headerEnd + 1) .. ($lines.Count - 1)]
		}else{$lines}

		foreach($line in $rawLines){
			# $split = Split-CommandChain $line
			# if($split.Count -gt 1){Validate-ChainNamespaces $split}
			# $commandLines += $split
			$commandLines += $line
		}

	}elseif($headerEnd -eq -1){$commandLines = $lines}

	# Process directives in remaining lines before tokenizing
	for($i = 0; $i -lt $commandLines.Count; $i++){
		$line = $commandLines[$i]
		if(Process-Directive $line){
			# mark directive lines as comments so tokenizer will skip them
			$commandLines[$i]= "# " + $line
		}
	}

	$tokens = Tokenize-Lines -Lines $commandLines -StartLine ($headerEnd + 2)
	$parser = [BuildParser]::new($tokens)
	$ast = $parser.ParseScript()
	$ast.Header = $header
	return $ast
}

# MOUNT STRING, Always call FRATEXE with this Command First
function FUN-CGCC{
	param(
		[AstCommand[]]$COMMAND
		# [string[]]$INPUTFILES,
		# [string]$OUTPUTFILE,
		# [string[]]$COMPILEARGS,
		# [string[]]$LINKARGS,
		# [bool]$LOGFILE,
		# [bool]$CACHE,
		# [bool]$DEBUG
	)
	foreach($COMMAND in $COMMANDS){
		$INPUTFILES = if($COMMAND.Flags["-f"]){$COMMAND.Flags["-f"]}else{$null}
		$OUTPUTFILE = if($COMMAND.Flags["-o"]){$COMMAND.Flags["-o"]}else{$null}
		$CARGS = if($COMMAND.Flags["-c"]){$COMMAND.Flags["-c"]}else{$null}
		$LARGS = if($COMMAND.Flags["-l"]){$COMMAND.Flags["-l"]}else{$null}
		$OUT = $null
		if($script:BuildFeatures.CACHE -and $script:BuildFeatures.DEBUG){
			$OUT = (& $CUSTOMGCC -INPUTFILES $INPUTFILES -OUTPUTFILE $OUTPUTFILE -COMPILEARGS $CARGS -LINKARGS $LARGS -CACHE -DEBUG)
		}
		if($script:BuildFeatures.CACHE){$OUT = (& $CUSTOMGCC -INPUTFILES $INPUTFILES -OUTPUTFILE $OUTPUTFILE -COMPILEARGS $CARGS -LINKARGS $LARGS -CACHE)}
		if($script:BuildFeatures.DEBUG){$OUT = (& $CUSTOMGCC -INPUTFILES $INPUTFILES -OUTPUTFILE $OUTPUTFILE -COMPILEARGS $CARGS -LINKARGS $LARGS -DEBUG)}
		Write-Log ($OUT -join "`n")
	}
}
function FUN-ECGCC{
	param(
		[string[]]$INPUTFILES,
		[string]$OUTPUTFILE,
		[string[]]$COMPILEARGS,
		[string[]]$LINKARGS,
		[bool]$LOGFILE,
		[bool]$CACHE,
		[bool]$DEBUG
	)
	$OUT = $null
	if($script:BuildFeatures.CACHE -and $script:BuildFeatures.DEBUG){
		$OUT = (& $CUSTOMGCC -INPUTFILES $INPUTFILES -OUTPUTFILE $OUTPUTFILE -COMPILEARGS $CARGS -LINKARGS $LARGS -CACHE -DEBUG)
	}
	if($script:BuildFeatures.CACHE){$OUT = (& $CUSTOMGCC -INPUTFILES $INPUTFILES -OUTPUTFILE $OUTPUTFILE -COMPILEARGS $CARGS -LINKARGS $LARGS -CACHE)}
	if($script:BuildFeatures.DEBUG){$OUT = (& $CUSTOMGCC -INPUTFILES $INPUTFILES -OUTPUTFILE $OUTPUTFILE -COMPILEARGS $CARGS -LINKARGS $LARGS -DEBUG)}
	Write-Log ($OUT -join "`n")
}

function FUN-PE2EXEC{
	param([AstCommand[]]$COMMANDS)
	$OUT = $false
	foreach($COMMAND in $COMMANDS){
		if(($COMMAND.Flags["-d"] -eq 1) -and $COMMAND.Flags["-i"] -and $COMMAND.Flags["-o"]){
			Write-Log "$($PE2EXEC) -d $($COMMAND.Flags["-i"]) $($COMMAND.Flags["-o"])"
			Write-Log "$((& $PE2EXEC '-d' $COMMAND.Flags["-i"] $COMMAND.Flags["-o"]) -join "`n")"
		}elseif(($COMMAND.Flags["-d"] -eq 1) -and $COMMAND.Flags["-i"]){
			Write-Log "$($PE2EXEC) -d $($COMMAND.Flags["-i"])"
			Write-Log "$((& $PE2EXEC '-d' $COMMAND.Flags["-i"]) -join "`n")"
		}elseif($COMMAND.Flags["-i"] -and $COMMAND.Flags["-o"]){
			Write-Log "$($PE2EXEC) $($COMMAND.Flags["-i"]) $($COMMAND.Flags["-o"])"
			Write-Log "$((& $PE2EXEC $COMMAND.Flags["-i"] $COMMAND.Flags["-o"] 2>&1) -join "`n")"
		}
		if((Test-Path $COMMAND.Flags["-o"]) -and $COMMAND.Flags["-o"]){continue}
		return $false
	}
	return $true
}
function FRAT-RUNTIME-START{
	$MOUNTCMD = $null
	$DISKPATH = if($AST.Header.'FS.IMAGE'){"-pp $($AST.Header.'FS.IMAGE')"}else{$null}
	$PARTITIONNAME = if($AST.Header.'FS.FORMAT'.'TARGET.PARTITION'){"-pt $($AST.Header.'FS.FORMAT'.'TARGET.PARTITION')"}else{$null}
	$BLOCKSIZE = if($AST.Header.'FS.FORMAT'.'TARGET.BLOCK.SIZE'){"-bs $($AST.Header.'FS.FORMAT'.'TARGET.BLOCK.SIZE')"}else{$null}
	$LOGBLOCKS = if($AST.Header.'FS.FORMAT'.'TARGET.LOG.BLOCKS'){"-lb $($AST.Header.'FS.FORMAT'.'TARGET.LOG.BLOCKS')"}else{$null}
	$VERSION = if($AST.Header.'FS.FORMAT'.'TARGET.VERSION'){"-v $($AST.Header.'FS.FORMAT'.'TARGET.VERSION')"}else{$null}
	if($DISKPATH){$MOUNTCMD = ",u $($DISKPATH) $($PARTITIONNAME) $($BLOCKSIZE) $($LOGBLOCKS) $($VERSION)"}

	$psi = New-Object System.Diagnostics.ProcessStartInfo
	$psi.FileName = $FSCONTROLLEREXEPATH
	$psi.Arguments = $null

	$psi.RedirectStandardInput  = $true
	$psi.RedirectStandardOutput = $true
	$psi.RedirectStandardError  = $true
	$psi.UseShellExecute        = $false
	$psi.CreateNoWindow         = $true
	$FSCONTROLLER.StartInfo = $psi
	$FSOUTEVENT = Register-ObjectEvent -InputObject $FSCONTROLLER -EventName "OutputDataReceived" -Action {
		if($EventArgs.Data){Write-Log -MSG "[SHELL OUT]: $($EventArgs.Data)" -COLOR Cyan}
		# This block runs automatically whenever the fs.exe program prints something
	}
	$FSERREVENT = Register-ObjectEvent -InputObject $FSCONTROLLER -EventName "ErrorDataReceived" -Action {
		if($EventArgs.Data){Write-Log -MSG "[SHELL ERR]: $($EventArgs.Data)" -COLOR Red}
	}

	$FSCONTROLLER.Start() | Out-Null
	$FSCONTROLLER.BeginOutputReadLine()
	$FSCONTROLLER.BeginErrorReadLine()
	$FSCONTROLLER.StandardInput.WriteLine($MOUNTCMD)
	Wait-Event -Timeout 0.5
	$script:FSSET = $true
}

function FUN-FRAT{
	param([AstBuildScript]$AST, [AstCommand[]]$COMMANDS)
	$TRUECMD = @()
	$MOUNTCMD = $null
	$DISKPATH = if($AST.Header.'FS.IMAGE'){"-p $($AST.Header.'FS.IMAGE')"}else{$null}
	$PARTITIONNAME = if($AST.Header.'FS.FORMAT'.'TARGET.PARTITION'){"-pt $($AST.Header.'FS.FORMAT'.'TARGET.PARTITION')"}else{$null}
	$BLOCKSIZE = if($AST.Header.'FS.FORMAT'.'TARGET.BLOCK.SIZE'){"-bs $($AST.Header.'FS.FORMAT'.'TARGET.BLOCK.SIZE')"}else{$null}
	$LOGBLOCKS = if($AST.Header.'FS.FORMAT'.'TARGET.LOG.BLOCKS'){"-lb $($AST.Header.'FS.FORMAT'.'TARGET.LOG.BLOCKS')"}else{$null}
	$VERSION = if($AST.Header.'FS.FORMAT'.'TARGET.VERSION'){"-v $($AST.Header.'FS.FORMAT'.'TARGET.VERSION')"}else{$null}
	if($DISKPATH){$MOUNTCMD = ",u $($DISKPATH) $($PARTITIONNAME) $($BLOCKSIZE) $($LOGBLOCKS) $($VERSION)"}
	foreach($COMMAND in $COMMANDS){
		switch($COMMAND.Name){
			'FSFRAT.MOUNT' {
				$IMAGEPATH = $(if($COMMAND.Flags["-p"]){"-p $($COMMAND.Flags["-p"])"}else{$null})
				$PARTITIONNAME = $(if($COMMAND.Flags["-pt"]){"-pt $($COMMAND.Flags["-pt"])"}else{$null})
				$BLOCKSIZE = $(if($COMMAND.Flags["-bs"]){"-bs $($COMMAND.Flags["-bs"])"}else{$null})
				$LOGBLOCKS = $(if($COMMAND.Flags["-lb"]){"-lb $($COMMAND.Flags["-lb"])"}else{$null})
				$VERSION = $(if($COMMAND.Flags["-v"]){"-v $($COMMAND.Flags["-v"])"}else{$null})
				if($IMAGEPATH){$TRUECMD += ",u $($IMAGEPATH) $($PARTITIONNAME) $($BLOCKSIZE) $($LOGBLOCKS) $($VERSION)"}
			} 'FSFRAT.ALIAS' {
				$ALIAS = $(if($COMMAND.Flags["-%"]){"-% $($COMMAND.Flags["-%"])"}else{$null})
				$DATA = $(if($COMMAND.Flags["-d"]){"-d $($COMMAND.Flags["-d"])"}else{$null})
				$NDATA = $(if($DATA){"-n $($DATA.Length.ToString())"}else{'-n 0'})
				if($ALIAS -and $DATA){$TRUECMD += ",s $($ALIAS) $($NDATA) $($DATA)"}
			} 'FSFRAT.OPEN' {
				$PATH = $(if($COMMAND.Flags["-p"]){"-p $($COMMAND.Flags["-p"])"}else{$null})
				$ALIAS = $(if($COMMAND.Flags["-a"]){"-a $($COMMAND.Flags["-a"])"}else{$null})
				$LOADARGS = $(if($COMMAND.Flags["-la"]){"-la $($COMMAND.Flags["-la"])"}else{$null})
				if($PATH -and $alias -and $LOADARGS){$TRUECMD += ",o $($PATH) $($ALIAS) $($LOADARGS)"}
			} 'FSFRAT.CLOSE' {
				$ALIAS = $(if($COMMAND.Flags["-a"]){"-a $($COMMAND.Flags["-a"])"}else{$null})
				if($ALIAS){$TRUECMD += ",cl $($ALIAS)"}
			} 'FSFRAT.DELETE' {
				$PATH = $(if($COMMAND.Flags["-p"]){"-p $($COMMAND.Flags["-p"])"}else{$null})
				if($PATH){$TRUECMD += ",d $($PATH)"}
			} 'FSFRAT.MAKE' {
				$PATH = $(if($COMMAND.Flags["-p"]){"-p $($COMMAND.Flags["-p"])"}else{$null})
				$ALIAS = $(if($COMMAND.Flags["-n"]){"-n $($COMMAND.Flags["-n"])"}else{$null})
				$LOADARGS = $(if($COMMAND.Flags["-la"]){"-la $($COMMAND.Flags["-la"])"}else{$null})
				if($PATH -and $ALIAS -and $LOADARGS){$TRUECMD += ",cr $($LOADARGS) $($PATH) $($ALIAS)"}
			} 'FSFRAT.READ' {
				$PPATH = $(if($COMMAND.Flags["-pp"]){"-pp $($COMMAND.Flags["-pp"])"}else{$null})
				$ALIAS = $(if($COMMAND.Flags["-ip"]){"-a $($COMMAND.Flags["-ia"])"}else{$null})
				$POS = $(if($COMMAND.Flags["-p"]){"-p $($COMMAND.Flags["-p"])"}else{'-p 0'})
				$NBYTES = $(if($COMMAND.Flags["-n"]){"-n $($COMMAND.Flags["-n"])"}else{'-n 0'})
				if($PPATH -and $ALIAS){$TRUECMD += ",fr $($PPATH) $($ALIAS) $($POS) $($NBYTES)"}
			} 'FSFRAT.WRITE' {
				$PPATH = $(if($COMMAND.Flags["-pp"]){"-pp $($COMMAND.Flags["-pp"])"}else{$null})
				$ALIAS = $(if($COMMAND.Flags["-ip"]){"-ip $($COMMAND.Flags["-ia"])"}else{$null})
				$POS = $(if($COMMAND.Flags["-p"]){"-p $($COMMAND.Flags["-p"])"}else{'-p 0'})
				$NBYTES = $(if($COMMAND.Flags["-n"]){"-n $($COMMAND.Flags["-n"])"}else{'-n 0'})
				if($PPATH -and $ALIAS){$TRUECMD += ",fw $($PPATH) $($ALIAS) $($POS) $($NBYTES)"}
			} default {break}
		}
	}
	if($script:BuildFeatures.'FSFRAT.RUNTIME'){
		if(-not $script:FSSET){(FRAT-RUNTIME-START)}
		foreach($CMD in $TRUECMD){
			Write-Log -MSG "$($CMD)" -COLOR Blue
			$FSCONTROLLER.StandardInput.WriteLine($CMD)
			Wait-Event -Timeout 0.5
		}
	}elseif($MOUNTCMD -and ($TRUECMD.Length -ge 1)){
		Write-Log "$($FSCONTROLLEREXEPATH) $($MOUNTCMD) $($TRUECMD -join ' ')"
		$FSOUT = (& $FSCONTROLLEREXEPATH $MOUNTCMD @TRUECMD) 2>&1
		Write-Log ($FSOUT -join "`n")
		return $true
	}
	return $false
}

if($HELP){
	(Show-Help)
	exit 0
}

# Pre-register built-in features and commands
Register-Feature -Name 'JSON' -Default $false
Register-Feature -Name 'FSFRAT.RUNTIME' -Default $true
Register-Feature -Name 'CACHE' -Default $false
Register-Feature -Name 'LOG' -Default $false
Register-Feature -Name 'DEBUG' -Default $false

Register-Command "CGCC.MAKE" 1 4 @{
	"-c" = @{Required = $true; Type="list"}
	"-f" = @{Required = $false; Type="list"}
	"-o" = @{Required = $true; Type="string"}
	"-l" = @{Required = $true; Type="list"}
}

Register-Command "SHELL.REBUILD" 0 0 @{}
Register-Command "SHELL.EXECUTE" 1 1 @{
	"-p" = @{Required=$true; Type="string"}
}

Register-Command "PE2EXEC.MAKE" 2 3 @{
	"-d" = @{Required=$false; Type="int"}
	"-i" = @{Required = $true; Type="string"}
	"-o" = @{Required = $true; Type="string"}
}
Register-Command "PE2EXEC.PRINT" 1 1 @{
	"-i" = @{Required = $true; Type="string"}
}

Register-Command "FSFRAT.ALIAS" 2 3 @{
	"-%" = @{Required=$true; Type="string"}
	"-n" = @{Required=$false; Type="string"}
	"-d" = @{Required=$true; Type="string"}
}
Register-Command "FSFRAT.MOUNT" 2 5 @{
	"-p" = @{Required=$true; Type="string"}
	"-pt" = @{Required=$true; Type="string"}
	"-bs" = @{Required=$false; Type="int"}
	"-lb" = @{Required=$false; Type="int"}
	"-v" = @{Required=$false; Type="string"}
}
Register-Command "FSFRAT.OPEN" 3 3 @{
	"-la" = @{Required=$true; Type="string"}
	"-p" = @{Required=$true; Type="string"}
	"-a" = @{Required=$true; Type="string"}
}
Register-Command "FSFRAT.CLOSE" 1 1 @{
	"-a" = @{Required=$true; Type="string"}
}
Register-Command "FSFRAT.DELETE" 1 1 @{
	"-p" = @{Required=$true; Type="string"}
}
Register-Command "FSFRAT.MAKE" 3 3 @{
	"-la" = @{Required=$true; Type="string"}
	"-p" = @{Required=$true; Type="string"}
	"-n" = @{Required=$true; Type="string"}
}
Register-Command "FSFRAT.READ" 4 4 @{
	"-pp" = @{Required=$true; Type="string"}
	"-ia" = @{Required=$true; Type="string"}
	"-p" = @{Required=$true; Type="int"}
	"-n" = @{Required=$true; Type="int"}
}
Register-Command "FSFRAT.WRITE" 4 4 @{
	"-pp" = @{Required=$true; Type="string"}
	"-ia" = @{Required=$true; Type="string"}
	"-p" = @{Required=$true; Type="int"}
	"-n" = @{Required=$true; Type="int"}
}


$AST = Parse-BuildFile $SHELLSCRIPT
(Open-Log $AST.Header.'LOG')
if(-not $AST){
	Write-Log "Could not generate AST from Script"
	exit 1
}

# Compile all Executables
foreach($EXECUTABLE in $AST.Header.'INPUT'.'EXECUTABLE'){
	$INFILES = $EXECUTABLE.'FILES'
	$OUTFILE = $EXECUTABLE.'OUTPUT'
	$CARGS = $EXECUTABLE.'CARGS'
	$LARGS = $EXECUTABLE.'LARGS'
	$GCCOUT = FUN-ECGCC -INPUTFILES $INFILES -OUTPUTFILE $OUTFILE -COMPILEARGS $CARGS -LINKARGS $LARGS
	Write-Log ($GCCOUT -join "`n")
}

$CMDPERLINE = @()
$LASTLINE = $AST.Commands[0].Line

# Group all AST commands by their line property automatically
$GroupedCommands = $AST.Commands | Group-Object Line

foreach($Group in $GroupedCommands){# .Group contains the array of commands belonging to this specific line
    $CMDPERLINE = $Group.Group 
    $PrimaryCmd = $CMDPERLINE[0]
    switch -Regex ($PrimaryCmd.Name){
        '^FSFRAT\..+$' {
            FUN-FRAT -AST $AST -COMMANDS $CMDPERLINE
			continue
        } '^PE2EXEC\..+$' {
            FUN-PE2EXEC -COMMANDS $CMDPERLINE
			continue
        } '^CGCC\..+$' {
            FUN-CGCC -COMMANDS $CMDPERLINE
			continue
        } '^SHELL.REBUILD$' {
            $PathFS = Join-Path (Get-Location) 'tools\filesystem\fs.ps1'
            $PathExec = Join-Path (Get-Location) 'tools\executable-format\exec.ps1'
            Write-Log "$((& $PathFS -RELEASE $true 2>&1) -join "`n")"
            Write-Log "$((& $PathExec 2>&1) -join "`n")"
			continue
        } '^SHELL.EXECUTE$' {
            $PATH = $null
            if($PrimaryCmd.Flags["-p"]){
                $TargetFlags = $PrimaryCmd.Flags["-p"]
                if(Test-Path $TargetFlags){$PATH = $TargetFlags}else{
                    $CleanedPath = $TargetFlags -replace '\.', ''
                    $ParentScriptDir = (Get-Item $SHELLSCRIPT).Parent.FullName
                    $ComboPath = Join-Path $ParentScriptDir $CleanedPath
                    if(Test-Path $ComboPath){$PATH = $ComboPath}
                }# Cleans up dots if relative path shorthand was used
            }
            if($PATH){Write-Log -MSG "$((& (Join-Path $TOOLSDIR 'tools\build-suite\build.ps1') -SHELLSCRIPT $PATH) -join "`n")"}
			continue
        }
    }
}

$FSCONTROLLER.StandardInput.WriteLine("exit")
$FSCONTROLLER.WaitForExit()
(Exit-PSSession)