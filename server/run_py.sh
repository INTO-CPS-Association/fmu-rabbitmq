pwd=$PWD
echo $pwd
osascript -e "tell app \"Terminal\" 
    do script \"cd $pwd; python3 consume.py\"
    do script \"cd $pwd; python3 playback_gazebo_data.py\"
    do script \"cd $pwd; python3 consume-systemHealthData.py\"
end tell"