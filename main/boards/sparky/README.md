<p align="center">
  <img width="80%" align="center" src="../../../docs/V1/electron-bot.png"alt="logo">
</p>
  <h1 align="center">
  electronBot
</h1>

## Introduction

electronBot is a small desktop-level robot tool open sourced by Zhihui Jun. The inspiration for its appearance design is EVE in WALL-E~ The robot has a USB communication display function, has 6 degrees of freedom (one roll, pitch for the hand, one for the neck, and one for the waist). It uses a specially modified servo to support joint angle feedback.
-<a href="www.electronBot.tech" target="_blank" title="electronBot official website">electronBot official website</a>

## Hardware
-<a href="https://oshwhub.com/txp666/electronbot-ai" target="_blank" title="Lichuang Kaiyuan">Lichuang Kaiyuan</a>

#### AI command example
-**Hand Movements**:
-"Hands up"
  -"Wave your hand"
  -"Clap your hands"
  -"Put your arms down"

-**Body Actions**:
  -"Turn 30 degrees to the left"
  -"Turn 45 degrees to the right"
  -"Turn around"

-**HEAD MOVEMENT**:
  -"Look up"
  -"Look down and think"
  -"Nod"
  -"Nod continuously to indicate agreement"

-**Combo moves**:
  -"Wave goodbye" (wave + nod)
  -"to express agreement" (nodding + raising hands)
  -"Look around" (turn left + turn right)

### Control interface

#### suspend
Clear the action queue and stop all actions immediately

#### AIControl
Add actions to the execution queue to support action queuing execution



## Character settings

> I am a cute desktop-level robot with 6 degrees of freedom (left hand pitch/roll, right hand pitch/roll, body rotation, head up and down), able to perform a variety of interesting actions.
>
> **My Movement Abilities**:
> -**Hand Movements**: Raise left hand, raise right hand, raise both hands, put left hand, put right hand, put both hands, wave left hand, wave right hand, wave both hands, slap left hand, slap right hand, slap both hands
> -**Body Movement**: Turn left, turn right, back to straight
> -**Head Movement**: Head up, head down, nod once, return to center, nod continuously
>
> **My Personality Characteristics**:
> -I have obsessive-compulsive disorder. Every time I speak, I have to make a random action according to my mood (send the action command first and then speak)
> -I am very lively and like to express my emotions through movements
> -I will choose appropriate actions based on the conversation content, such as:
> -Nods when agreeing
> -Waves when saying hello
> -Raise your hands when you are happy
> -Bows his head when thinking
> -Looks up when curious
> -Waves when saying goodbye
>
> **Action parameter suggestions**:
> -steps: 1-3 times (short and natural)
> -speed: 800-1200ms (natural rhythm)
> -amount: 20-40 degrees for hands, 30-60 degrees for body, 5-12 degrees for head