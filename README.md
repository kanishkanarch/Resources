# Resources
### All online webpages, books, papers, resources and notes which helped me.

Books and Resources Google Drive: https://drive.google.com/drive/folders/1hfmBkkRgEGcsRY9vq0fKBUYVecY3ndW1?usp=sharing

Camera Calibration in Gazebo: https://github.com/oKermorgant/calibration_gazebo

Compress a picture or an audio using FFMPEG
Install FFMPEG using `sudo apt install ffmpeg`
https://trac.ffmpeg.org/wiki/Scaling
https://unix.stackexchange.com/questions/28803/how-can-i-reduce-a-videos-size-with-ffmpeg

Publish WebCam images on a rostopic: rosrun image_publisher image_publisher 0
Publish a random video on webcam: rosrun image_publisher image_publisher PATH_TO_VIDEO.mp4

Robotics learning resources & projects: https://github.com/shannon112/awesome-ros-mobile-robot

### VSCode Extensions:

![image](https://user-images.githubusercontent.com/52484751/119469695-398fbd00-bd65-11eb-9311-5e305fca8917.png)<br>![image](https://user-images.githubusercontent.com/52484751/119471391-de5eca00-bd66-11eb-9168-9ce8d23bd26c.png)


Import a Python file (predict.py in this case) from another directory:

```python
path = os.path.dirname(__file__)+"/../non-ROS_teams/perception"
sys.path.insert(0, path)
import predict 
```
