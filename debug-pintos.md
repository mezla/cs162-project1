# How to Debug Pintos

### Pull latest docker image from docker.io
    
  ```Shell
    sudo docker pull thinkhy/cs162-pintos
  ``` 

### Run container
   
   ```Shell
   sudo docker run -i -t -v [pintosSRC]/pintos:/pintos docker.io/thinkhy/cs162-pintos  bash
   ```

### Start GDB server   
 
  ```Shell
  cd /pintos
  ./run.sh
  ```
  
### Create another session to start a GDB client

  ```Shell
  sudo docker ps  # find the container ID just run
  sudo docker exec -t -i [ContainerID] bash
  # at this point, you are in the container
  cd /pintos/src/thread/build
  gdb ./kernel.o
  # in GDB
  target remote localhost:1234
  continue
  ```
  
Happy Coding!
   
