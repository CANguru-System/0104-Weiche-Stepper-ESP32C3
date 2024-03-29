
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

#include <Stepper.h>

// ****** doubleclick
// das System befindet sich in der Anlaufphase (phase0), i.e. Neuinstallation oder Zurücksetzen des Decoders.
// startet die Prozedur, mit der der Laufweg des Steppers eingestellt wird.
// Nach dem doubleClick
// phase1:
// läuft der Stepper vom Motor weg. Ist der Stepper am Ende angekommen, wird
// ein singleClick
// phase2:
// gedrückt. Daraufhin läuft der Stepperarm in Richtung Motor. Ist er an diesem
// Ende angekommen, wird erneut
// ein singleClick
// phase3:
// gedrückt und der Motor läuft in die Anfangsposition auf der motorabgewandten Seite.
// dort angekommen beginnt erneut phase0 (Betriebsphase)

// ****** longClick
// das System befindet sich in der Anlaufphase (phase0), i.e. Neuinstallation oder Zurücksetzen des Decoders.
// startet eine Prozedur, in der der Stepper in die Mitte des Laufweges verschoben werden kann.
// Nach dem longClick (loslassen des Knopfes):
// phase4:
// A.: doubleCick:
// phase5:
// Stepper läuft in Richtung Motor, Nach einem
// singleClick:
// phase6: bleibt der Motor stehen, dann wieder phase0.
// B.: singleClick:
// phase6:
// Stepper läuft vom Motor weg. Nach einem
// singleClick:
// phase6: bleibt der Motor stehen, dann wieder phase0.

// Voreinstellungen, steppernummer wird physikalisch mit
// einem stepper verbunden

/*  switch (phase)
  {
  case phase0:
    // waiting for initial button
    if (btndebounce())
    {
    }
    if (btn_Correction.debounce())
    {
      // step one revolution in one direction:
      log_d("going to Correction");
      phase = phase4;
      no_correction = false;
    }
    break;

  case phase1:
    break;

  case phase2:
    break;
  }
*/
// this function will be called when the button was pressed a single time.

bool bcontinue;

void setContinue(bool c)
{
  bcontinue = c;
}

bool getContinue()
{
  return bcontinue;
}

void StepperwButton::runReverse()
{
  // läuft in Richtung der zum Motor entgegengesetzten Ende
  direction = reverse;
  step = steps - 1;
  // run to one end till button is pressed
  // step one revolution in one direction:
  // move only if the appropriate delay has passed:
  last_step_time = 0;
  while (getContinue())
  {
    now_micros = micros();
    if (now_micros - last_step_time >= step_delay)
    {
      // get the timeStamp of when you stepped:
      last_step_time = now_micros;
      oneStep();
    }
  }
} // runReverse

void StepperwButton::runForward()
{
  // läuft in Richtung des Motors
  direction = forward;
  step = 0;
  // run to the opposite end till button is pressed
  // step one revolution in one direction:
  last_step_time = 0;
  while (getContinue())
  {
    now_micros = micros();
    if (now_micros - last_step_time >= step_delay)
    {
      // get the timeStamp of when you stepped:
      last_step_time = now_micros;
      oneStep();
      stepsToSwitch++;
    }
  }
} // runReverse

void StepperwButton::singleClick()
{
  // gedrückt wird der Knopf an der roten LED
  log_d("singleClick");
  setContinue(true);
  switch (phase)
  {
  case phase0:
    // läuft in Richtung Ende
    log_d("measuringphase: running to the end");
    readyToStep = false;
    phase = phase1;
    runReverse();
    break;
  case phase1:
    // läuft in Richtung Motor:
    log_d("measuringphase: stops and runs to the motor");
    phase = phase2;
    delay(direction_delay);
    stepsToSwitch = 0;
    stopStepper();
    runForward();
    break;
  case phase2:
    // stoppt und läuft zurück zum anderen Ende und stoppt
    log_d("measuringphase: stops and runs to the end");
    stopStepper();
    readyToStep = true;
    leftpos = stepsToSwitch;
    phase = phase0;
    acc_pos_curr = left;
    GoLeft();
    currpos = 0;
    set_stepsToSwitch = true;
    break;
  case phase3:
    log_d("correctionphase: runs to the motor");
    // läuft in Richtung Motor
    phase = phase4;
    runForward();
    break;
  case phase4:
    log_d("correctionphase: stops running");
    // beendet den Lauf nach Korrekturphase
    stopStepper();
    phase = phase0;
    break;
    }
} // singleClick

// this function will be called when the button was pressed 2 times in a short timeframe.
void StepperwButton::doubleClick()
{
  log_d("correctionphase");
  setContinue(true);
  if (phase == phase3)
  {
    log_d("correctionphase: runs to the end");
    // läuft in Richtung Ende
    phase = phase4;
    runReverse();
  }
  if (phase == phase0)
  {
    // startet die Korrekturphase
    log_d("correctionphase: start");
    phase = phase3;
  }
} // doubleClick

void StepperBase::Attach()
{
  // setup the pins on the microcontroller:
  pinMode(A_plus, OUTPUT);
  pinMode(A_minus, OUTPUT);
  pinMode(B_plus, OUTPUT);
  pinMode(B_minus, OUTPUT);
  stopStepper();
  last_step_time = 0;
  direction_delay = 250; // 20 * step_delay/1000;
  phase = phase0;
  readyToStep = stepsToSwitch != 0;
  set_stepsToSwitch = false;
  no_correction = true;
  rightpos = 0;
  leftpos = stepsToSwitch;
  switch (acc_pos_curr)
  {
  case right:
    currpos = rightpos;
    break;
  case left:
    currpos = leftpos;
    break;
  }
}

void StepperBase::oneStep()
{
  switch (step)
  {
    // Bipolare Ansteuerung Vollschritt
    // 1a 1b 2a 2b
    // 1  0  0  1
    // 0  1  0  1
    // 0  1  1  0
    // 1  0  1  0

  case 0: // 1  0  0  1
    digitalWrite(A_plus, HIGH);
    digitalWrite(A_minus, LOW);
    digitalWrite(B_plus, LOW);
    digitalWrite(B_minus, HIGH);
    break;
  case 1: // 0  1  0  1
    digitalWrite(A_plus, LOW);
    digitalWrite(A_minus, HIGH);
    digitalWrite(B_plus, LOW);
    digitalWrite(B_minus, HIGH);
    break;
  case 2: // 0  1  1  0
    digitalWrite(A_plus, LOW);
    digitalWrite(A_minus, HIGH);
    digitalWrite(B_plus, HIGH);
    digitalWrite(B_minus, LOW);
    break;
  case 3: // 1  0  1  0
    digitalWrite(A_plus, HIGH);
    digitalWrite(A_minus, LOW);
    digitalWrite(B_plus, HIGH);
    digitalWrite(B_minus, LOW);
    break;
  }
  switch (direction)
  {
  case forward:
    step++;
    if (step >= steps)
      step = 0;
    break;
  case reverse:
    step--;
    if (step < 0)
      step = steps - 1;
    break;
  }
}

void StepperBase::stopStepper()
{
  digitalWrite(A_plus, LOW);
  digitalWrite(A_minus, LOW);
  digitalWrite(B_plus, LOW);
  digitalWrite(B_minus, LOW);
}

// Setzt die Zielposition
void StepperBase::SetPosition()
{
  last_step_time = micros();
  acc_pos_dest = acc_pos_curr;
  switch (acc_pos_dest)
  {
  case left:
  {
    GoLeft();
  }
  break;
  case right:
  {
    GoRight();
  }
  break;
  }
}

// Zielposition ist links
void StepperBase::GoLeft()
{
  log_d("going left");
  destpos = leftpos;
  // von currpos < 74 bis 74, des halb increment positiv
  increment = 1;
  step = 0;
  direction = reverse;
  /*  endpos = -maxendpos; // +1
    way = longway;*/
}

// Zielposition ist rechts
void StepperBase::GoRight()
{
  log_d("going right");
  destpos = rightpos; // 1
                      // von currpos > 1 bis 1, des halb increment negativ
  increment = -1;
  step = steps - 1;
  direction = forward;
  /*  endpos = maxendpos; // -1
    way = longway;*/
}

// Überprüft periodisch, ob die Zielposition erreicht wird
void StepperwButton::Update()
{
  handle();
  if (readyToStep && (destpos != currpos))
  {
    if (micros() - last_step_time >= step_delay)
    {
      // get the timeStamp of when you stepped:
      last_step_time = micros();
      oneStep();
      currpos += increment;
      if (destpos == currpos)
        stopStepper();
    }
  }

  /*
    // Überprüfung, ob die Zielposition bereits erreicht wurde
    if (pos != (destpos + endpos))
    {
      // wenn die Zeit updateInterval vorüber ist, wird
      // wird das stepper ggf. neu gestellt
      if ((micros() - lastUpdate) > updateInterval)
      {
        // time to update
        currpos += increment;
  //      ledcWrite(channel, pos);
        lastUpdate = micros();
      }
    }
    else
    {
      // Zielposition wurde erreicht
      if (way == longway)
      {
        // Überschwingpunkt jetzt rückwärts zum Zielpunkt laufen
        increment *= -1;
        // kein Überschwingen mehr
        endpos = 0;
        // Umschalten auf noway
        way = noway;
      }
    }*/
}
