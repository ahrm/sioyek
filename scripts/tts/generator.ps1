Add-Type -AssemblyName System.Speech;
$synth = New-Object System.Speech.Synthesis.SpeechSynthesizer;
$text = Get-Content $args[0]
$synth.SetOutputToWaveFile($args[1])
$synth.Speak($text);
