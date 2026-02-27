void setup() {
  // put your setup code here, to run once:
  pinMode(2, INPUT);
  pinMode(1, OUTPUT);
  pinMode(35, OUTPUT);


}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.begin(115200); //9600
  int analogValue = analogRead(2); 
  
  Serial.println(analogValue);
  if(analogValue>2000){
    //room is dark, turn light on
    digitalWrite(1, HIGH);
    digitalWrite(35, LOW);


  }
  else{
    //room is lighted up, turn light off
        digitalWrite(1, LOW);
        digitalWrite(35, HIGH);

  }



  


  delay(100);

 //4095
 
 //10
}
