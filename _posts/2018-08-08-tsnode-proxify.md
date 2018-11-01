---
layout: post
title: Proxy-based function hook and AOP library for node.js 
categories: [node.js]
tags: [node.js, aop, typescript, javascript, proxy]
fullview: true
comments: true
---

## Introduction

[`tsnode-proxify`](https://github.com/leezhenghui/tsnode-proxify.git) is a proxy-based method hooks and AOP library for [node.js](https://nodejs.org) with [typescript](https://www.typescriptlang.org/). It allows you to extend/provide customized QoS handler and apply these QoS features via typescript decorators(metadata-programming-like syntax) without invasiveness to existing code logic, which increase modularity for your application.

> ![Note]({{ site.url }}/assets/ico/note.png)
> 
> tsnode-proxify depends on typescript decorator feature which is marked as **experimental** feature in typescript. In the meanwhile, tsnode-proxify is still a **beta** version project for now. 

## Background

Before we jump into the tsnode-proxify, let's take one step back to take a look what problems could be resolved by proxy pattern and AOP, why it is so important in JEE.

### Proxy pattern 

The `proxy` is a well-used design pattern. We can see it in either high level software architecture design, e.g: api-gateway and service mesh in microservices, or a narrow-down specific programming module. Generally speaking, the proxy pattern provides the capablity to implement a contract interface, but adds special functionality on-the-fly behind the sense. 

### AOP

[`Aspect Oriented Programming`](https://en.wikipedia.org/wiki/Aspect-oriented_programming)(AOP) addresses the problem of cross-cutting concerns. It is a complement to Object-Oriented-Programming(OOP). Not like OOP, which provide class as an key unit of modularity, AOP focus on `aspect` as it's unit of modularity, the `aspect` extract and modularization the code which cut across multiple classes/components for the same concern solution in a system. Such concerns are often termed cross-cutting concerns in AOP literature. In a real-life system, the typical cross-cutting concerns includes logging, security, transaction management, caching, validation, etc. Providing AOP capability is important to increase modularity, and normally be a fundamental feature in a middleware platform.  

#### AOP in Java 

AOP is really popular in java. In a JEE runtime, the transaction, security are noramlly provided as AOP aspects under the hood. In SOA, the SCA runtime also heavily depends on the AOP to provide QoS, IT-Specific features around the business logic. In Spring, the AOP is actually delivered as a base component in fundamental layer and open to upper stack. Indeed, per my experiences on JEE server development, AOP provides an excellent solution for the problems in enterprise application field to increase modularity and make the system more loose-coupled, especially in the middleware product development.

Implementing an AOP framework to advise method execution, the proxy pattern is perfect fit here. The aspect module code can be abstracted, prepared and  **injected** at `before` and `after` points of the method and also be able to recieve the execution context, arguments and output(or fault) to the target operation like it was there.  That is reason we usually see the proxy pattern in an AOP framework.  In a pure java world, some typical approaches to achieve this:

- Weave(static-way)
  - Compile-time weaving(e.g: using AspectJ compiler, which can provide complete AOP implementation)
  - Load-time weaving(e.g: AspectJ LTW, which depends on the JVM option -javaagent, provide complete AOP implementation)
- JDK Proxy(dynamical-way, for java interface and method execution join point only)
- CGlib Proxy(dynamical-way, adopt to both class and interface, method execution join point only, but need the proxied method to be public)

> ![Note]({{ site.url }}/assets/ico/note.png)
> 
> In a pure java world, using dynamical proxy way for AOP implementation, it usually be used in conjunction with a IoC container, which can take over the responsibility of proxy instance construction and injection. Hide these implementation details and provide a clean/easy-to-use programming model to developer.

## Motivation

As mentioned above, the AOP framework can bring us so many benefits to improve the software modularity. Inspired by JEE experiences, I believe it would be helpful if we have similar framework in node.js. That is reason I come across to tsnode-proxify project. The goal of tsnode-proxify is NOT to provide a complete AOP implementation, it primarily focus on method execution join point for the moment, will consider some IoC features to enable the property injection of proxied object in next step. 

## Concepts

Before we dig into the tsnode-proxify, we need to clarify some concepts. 

1. **Interaction style** for a method call in node.js

    - `Sync`: The method completion should be the same time point as method invocation being returned. 

     ```
     e.g: 
     greet(name: string): string {
       return 'Hello, ' + name;
     }
     ```
    
    - `Async`: The method completion will be done in a future certain time point after the moment of method invocation being returned. 

     ```
     e.g: 
     greet(name: string, cb: Function): void{
       setTimeout(function() {
         let reval = 'Hello, ' + name;
         cb(null, reval);
       }, 10); 
     }
     ```

2. **Completion hints**: invocation completion hint is the concept relevant to how tsnode-proxify runtime understand `after` of an execution join point. It should be mark as the moment of method logic execution done.

    - target method getting returned (of course, this is for sync style method only)
    
    - the callback method getting called (adopt to both sync and async style method)
    
    - the returned promise's status change from pending to resolved or rejected (for async method with promise as return value only)  

3. [tsnode-proxify](https://github.com/leezhenghui/tsnode-proxify.git) can support below **interaction style and completion hints combinations** with `before` and `after` advise join points

    - `sync-return`
    
    - `sync-callback`
    
    - `async-callback`
    
    - `async-promise`

`tsnode-proxify` enable the aspect modularity to be implemented as an `Interceptor` class(declared by @Interceptor decorator) for a specific QoS intention, which can be dynamically injected into the join-point if a desired @QoS declaration being claimed on the target method. 

```typescript

@Interceptor({
  "interactionStyle": InteractionStyleType.SYNC
})
  class NoopInterceptor extends AbstractInterceptor{
  constructor(config: any) {
  	super(config);	
  }
  
  // called before get into before and after advise
  public canProcess(context: InvocationContext, callback: (error: any, canProcess: boolean) => void): void {
  	callback(null, true);	
  }
  
  // being called at before advise
  public handleRequest(context: InvocationContext, done: Function): void {
  	done();	
  }
  
  // being called at after advise with output
  public handleResponse(context: InvocationContext, done: Function): void {
  	done();	
  }
  
  // being called at after advise with fault 
  public handleFault(context: InvocationContext, done: Function): void {
  	done();	
  }
  
  public getName(): string {
  	return 'NoopInterceptor';	
  }
}

```

> ![Note]({{ site.url }}/assets/ico/note.png)
> 
> Notable, If the target method is **sync** interaction style(especially, expected a synchronous return value), all of interceptors applied to that method should also be **sync** interaction style. 

### Quick Start

- Prerequsites 
  - node.js 
  - typescript toolkits(including tsc and ts-node command)
  - [Q](https://github.com/kriskowal/q) based promise, will support native promise soon.

> ![Note]({{ site.url }}/assets/ico/note.png)
> If you don't want to following this step-by-step guide to complete the helloworld sample, you can download the sample from [link](https://github.com/leezhenghui/hello-tsnode-proxify.git) for a quick start
>

- Create your package folder e,g: named **hello-tsnode-proxify**

  ```shell
  mkdir hello-tsnode-proxify
  cd hello-tsnode-proxify
  mkdir src
  git init
  echo "# My first sample for tsnode-proxify" >> README.md
  git add . && git commit -m "initial commit"
  ```

- Create **package.json**

  ```
  npm init -y 
  echo "node_modules" >> .gitignore
  echo "dist" >> .gitignore
  echo "package-lock.json" >> .gitignore
  npm install --save-dev typescript
  npm install --save tsnode-proxify
  ```
- Create **tsconfig.json**

  ```
  {
    "compilerOptions": {
      "target": "es5",
      "module": "commonjs",
      "declaration": true,
      "outDir": "./dist",
      "experimentalDecorators": true
    },
    "include": ["src"],
    "exclude": ["node_modules"]
  }
  ```
- Add scripts to package.json

  ```
  "build": "tsc",
  "start": "node dist/helloworld.js",
  "main": "dist/helloworld.js"
  ```
  
- Create a logger interceptor in file **src/logger.ts** as below:

  ```
  import {
    Interceptor,
    InteractionStyleType,
    AbstractInterceptor,
    InvocationContext,
  } from 'tsnode-proxify';
  
  @Interceptor({
    interactionStyle: InteractionStyleType.SYNC,
  })
  export class Logger extends AbstractInterceptor {
    private LOG_PREFIX: string = '[logger] ';
  
    constructor(config: any) {
      super(config);
    }
  
    private getTargetFullName(context: InvocationContext): string {
      let targetFullName = context.getClassName() + '.' + context.getOperationName();
  
      return targetFullName;
    }
  
    public init(context: InvocationContext, done: Function): void {
      console.log(this.LOG_PREFIX + '<init> ');
      done();
    }
  
    public handleRequest(context: InvocationContext, done: Function): void {
      console.log(
        this.LOG_PREFIX +
          '<request> ' +
          this.getTargetFullName(context) +
          '; [input]: "' +
          context.input +
          '"; [timestamp]: ' +
          new Date().getTime(),
      );
      done();
    }
  
    public handleResponse(context: InvocationContext, done: Function): void {
      console.log(
        this.LOG_PREFIX +
          '<response> ' +
          this.getTargetFullName(context) +
          '; [output]: "' +
          context.output +
          '"; [timestamp]: ' +
          new Date().getTime(),
      );
      done();
    }
  
    public handleFault(context: InvocationContext, done: Function): void {
      console.log(
        this.LOG_PREFIX +
          '<fault> ' +
          this.getTargetFullName(context) +
          '; [fault]: ' +
          context.fault +
          '; [timestamp]: ' +
          new Date().getTime(),
      );
      done();
    }
  
    public canProcess(context: InvocationContext, callback: (error: any, canProcess: boolean) => void): void {
      callback(null, true);
    }
  
    public getName(): string {
      return 'Logger';
    }
  }
  ```

  To create a interceptor which encapsulate an cross-cutting concern solution, you just need to implement a class, which

  - @Interceptor decorator: declare a class to be registried as interceptor in tsnode-proxify runtime
  - Extend the **AbastractInterceptor** base class, and implement the methods.

- Create a component in file **src/helloworld.ts** as below:

  As you see beow, to "declare" a class to be a component managed, we just add to apply a couple of decorators to it. No other differences than a normal class definition.
  
  ```
  import {
    Component,
    QoS,
    InteractionStyle,
    Completion,
    Callback,
    Fault,
    Output,
    InteractionStyleType,
    InvocationContext,
  } from 'tsnode-proxify';
  import { Logger } from './logger';
  
  @Component()
  class Hello {
    constructor() {}
  
    @InteractionStyle(InteractionStyleType.SYNC)
    @QoS({ interceptorType: Logger })
    greet(name: string): string {
      console.log('[greet]    ==> I am saying hello to', name);
      return 'Hello, ' + name;
    }
  }
  
  // =====================
  //    main
  // ====================
  
  let hello: Hello = new Hello();
  console.log('[result]: "' + hello.greet('World') + '"');
  ```

  - @Component decorator: declare a class to be managed as a component in tsnode-proxify  
  - @QoS decorator: declare a method to be proxify and provide `before` and `after` advises  

> ![Note]({{ site.url }}/assets/ico/note.png)
> 
> Notable, to keep the helloworld sample as simple as possible, I don't introduce some other decorators in that sample. If you want to try @Completion and callback invocation, you can refer to [stock](https://github.com/leezhenghui/tsnode-proxify/tree/master/demo/stock.ts) sample. For more advanced usages, please refer to integration unit test cases.

- Run the helloworld sample 

  ```shell
  npm run build
  npm run start
  ```

- The output result with aspect logger feature:

  ```
  > hello-tsnode-proxify@1.0.0 start /home/lizh/tmp/hello-tsnode-proxify
  > node dist/helloworld.js
  
  [logger] <init> 
  [logger] <request> Hello.greet; [input]: "World"; [timestamp]: 1541054935419
  [greet]    ==> I am saying hello to World
  [logger] <response> Hello.greet; [output]: "Hello, World"; [timestamp]: 1541054935420
  [result]: "Hello, World"
  ```

### Run Unit Tests

You can run the unit tests to get a full picture of what tsnode-proxify support so far.

```shell
npm run test

> mocha --compilers ts:ts-node/register,tsx:ts-node/register ./test/**/*test.ts

    ...

Integration Tests
   ✓ @QoS on static sync-return method with sync-interceptor
   ✓ @QoS on sync-return method with sync-interceptor
   ✓ @QoS on sync-callback method with sync-interceptor
   ✓ @QoS on async-promise method with sync-interceptor (101ms)
   ✓ @QoS on async-promise method with async-interceptor (101ms)
   ✓ @QoS on async-callback method with sync-interceptor (100ms)
   ✓ @QoS on async-callback method with async-interceptor (251ms)
   ✓ @QoS on sync-callback method which being invoked recursively with a pass-through callback function handler
   ✓ @QoS on a method with recursive invocations, QoSed method is triggered by "this" reference
   ✓ @QoS on sync-return bind()ed method
   ✓ Validation: async style interceptor can NOT be applied to sync style target method
   ✓ Validation: conflict/dumplicated interceptor names
```

## Join us

If you are interested in this project, please feel free to let me know,  any bug-report/comments/suggestions/contribution on this project is appreciated. :-)
