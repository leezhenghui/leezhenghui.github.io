---
layout: post
title: Prototype of porting Apache Aries OSGi blueprint container to Android 
categories: [OSGi]
tags: [java, OSGi, android]
comments: true
---

### Table of Contents

* Kramdown table of contents
{:toc .toc}

> *Move my earlier post(see original link [here](http://mail-archives.apache.org/mod_mbox/aries-dev/201101.mbox/%3CAANLkTi=xEX4OeDE_3MUUFUgvFk31x-f3nX+Ke_JECqJh@mail.gmail.com%3E) created on Sun, 23 Jan 2011) to the new techblog*

Today, I gave a try on running aries blueprint on android. now I can get the
helloworld sample work on it. Below is a brief summary about  my experiment.

### Intention

Aim to know what we need to do if we want to enable aries blueprint
implementation on android's dalvik

### Environment

- OS: ubuntu 10.10 32bit
- OSGi framework: Felix(version 3.0.7)
- Aries blueprint: The latest code from trunk branch.
- Android SDK: 2.3 with API9

### Brief description about the steps
1. I’m an enthusiastic user of the best IDE -- e.g: Eclipse, so the
firstly thing for me is to construct PDE projects for aries blueprint, aries
proxy and aries util. Currently, I do this manually for now, :-( if Aries
can provide a maven plugin to help do this, I believe most of bundle dev
will like it.

2. To make things simple, I use a mock pax logger to replace the pax
logger

3. For the aries.proxy, we need to force it run into JDKProxy path,
since the Dalvik can not recognize java's bytecode. The current aries.proxy
can support this via failing access to ASM class. but seems the MANIFEST
does not give a "optional attribute" for the import-package description to
ASM packages. then I added "resolution:=optional" for ASM dependences.
        For aries blueprint project, seems it also depends on some other
projects -- blueprint annotation and quiesce, because I just want to run a
"helloworld" sample, mark them to resolution optional in MANIFEST.

4. Modify the Felix properties to include the javax.xml..... to system
ext package list.

5. I encountered an issue on xml validation function. I looked into
this issue and found perhaps, the android platform does not provide a
build-in SchemaFactory implementation(it is interesting, per spec, platform
who provide the javax.xml.validation, must provide an implementation for the
W3C schema, but seems android does not have it.) After googled,  I found
there is a defect opened against to android --
http://code.google.com/p/android/issues/detail?id=9491. So I made some
cracker code to change xmlValidation flag to false(by default , it is true)
in blueprint to make things go ahead.

6. Build the bundle projects with Android2.3 libraries, make sure there
is NO error during compiling. and run below command to convert a jar to .dex
and repackage the jar.

```
  <Anroid_SDK>/platform-tools/dx --dex --output=<your dex> <your
jar>
  <Anroid_SDK>/platform-tools/aapt add <your jar> <your dex>
```

7. To control/debug the bundle easily, develop an android application
which can launch Felix system framework and manage the bundles.

8. Deploy the launcher application on android, and install&start the
bundles from the sdcard directory.

### Result

Observe the log via command adb logcat, and we can see below result:

```stdout
01-22 17:38:36.746: DEBUG/dalvikvm(406): DexOpt: --- BEGIN 'bundle.jar'
(bootstrap=0) ---
01-22 17:38:37.046: DEBUG/dalvikvm(645): DexOpt: load 19ms, verify+opt 19ms
01-22 17:38:37.096: DEBUG/dalvikvm(406): DexOpt: --- END 'bundle.jar'
(success) ---
01-22 17:38:37.305: INFO/System.out(406): ======>>> Starting HelloWorld
Server
01-22 17:38:37.355: INFO/System.out(406):
[org.apache.aries.blueprint.container.ServiceRecipe] [debug] {null;null;}
01-22 17:38:37.385: INFO/System.out(406):
[org.apache.aries.blueprint.container.ServiceRecipe] [debug] ### Message
###: Creating service instance{}
01-22 17:38:37.455: INFO/System.out(406):
[org.apache.aries.blueprint.container.ServiceRecipe] [debug]
{org.apache.aries.samples.blueprint.helloworld.server.HelloWorldServiceImpl@405f1088
;}
01-22 17:38:37.485: INFO/System.out(406):
[org.apache.aries.blueprint.container.ServiceRecipe] [debug] ### Message
###: Creating listeners{}
01-22 17:38:37.555: INFO/System.out(406):
[org.apache.aries.blueprint.container.ServiceRecipe] [debug] {[];}
01-22 17:38:37.585: INFO/System.out(406):
[org.apache.aries.blueprint.container.ServiceRecipe] [debug] ### Message
###: Calling listeners for initial service registration{}
01-22 17:38:38.425: INFO/System.out(406):
[org.apache.aries.blueprint.container.BlueprintEventDispatcher] [debug]
{BlueprintEvent[type=CREATED];org.apache.aries.samples.blueprint.helloworld.server;}
01-22 17:38:53.836: INFO/System.out(406):
[org.apache.aries.blueprint.container.ReferenceRecipe] [debug]
{helloservice;[org.apache.aries.samples.blueprint.helloworld.api.HelloWorldService];}
01-22 17:38:53.906: DEBUG/dalvikvm(406): DexOpt: --- BEGIN 'bundle.jar'
(bootstrap=0) ---
01-22 17:38:54.136: DEBUG/dalvikvm(652): DexOpt: load 10ms, verify+opt 16ms
01-22 17:38:54.177: DEBUG/dalvikvm(406): DexOpt: --- END 'bundle.jar'
(success) ---
01-22 17:38:54.345: INFO/System.out(406): ========>>>>Client HelloWorld:
About to execute a method from the Hello World service
01-22 17:38:54.375: INFO/System.out(406):
[org.apache.aries.blueprint.container.ServiceRecipe] [debug]
{org.apache.aries.samples.blueprint.helloworld.client
[11];org.apache.felix.framework.ServiceRegistrationImpl@40674538;}
01-22 17:38:54.395: INFO/System.out(406):
[org.apache.aries.blueprint.container.ServiceRecipe] [debug]
{getService;org.apache.aries.samples.blueprint.helloworld.server.HelloWorldServiceImpl@405f1088
;}
01-22 17:38:54.405: INFO/System.out(406): ======>>> A message from the
server: Hello World!
01-22 17:38:54.415: INFO/System.out(406): ========>>>>Client HelloWorld: ...
if you didn't just see a Hello World message something went wrong
01-22 17:38:55.096: INFO/System.out(406):
[org.apache.aries.blueprint.container.BlueprintEventDispatcher] [debug]
{BlueprintEvent[type=CREATED];org.apache.aries.samples.blueprint.helloworld.client;}
```
