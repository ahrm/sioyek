$text = Get-Content $args[0]
 tts --text "$text" --use_cuda USE_CUDA --out_path $args[1]
