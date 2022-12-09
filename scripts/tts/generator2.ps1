$text = Get-Content $args[0]
if (Get-Command "nvidia-smi" -errorAction SilentlyContinue) {
    tts --text "$text" --use_cuda USE_CUDA --out_path $args[1]
} else {
    tts --text "$text" --out_path $args[1]  
}
