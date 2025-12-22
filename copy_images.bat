: %1 Source dir
: %2 Build config name
: %3 Output dir

set source_dir=%1
set build_config_name=%2
set output_dir=%3

cd %source_dir:/=\%
: copy images to exe path
xcopy /y /i /s .\images %output_dir:/=\%\images