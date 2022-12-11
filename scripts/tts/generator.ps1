$text = Get-Content $args[0]
if ($isMacOS) {
    say $text -o $args[1]
} else {
    Add-Type -AssemblyName System.Speech;
    $synth = New-Object System.Speech.Synthesis.SpeechSynthesizer;
    $synth.SetOutputToWaveFile($args[1])
    $synth.Speak($text);
}
