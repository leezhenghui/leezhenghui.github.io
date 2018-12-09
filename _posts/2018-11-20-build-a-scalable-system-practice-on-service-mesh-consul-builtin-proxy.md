---
layout: post
title: Build a Modern Scalable System - Practice on Service Mesh Mode with Consul and Nomad 
categories: [microservices]
tags: [architecture, microservices, java, nomad, consul, servicemesh]
series: [build-scalable-system]
comments: true
---

{% include common/series.html %}

## Table of Contents

* Kramdown table of contents
{:toc .toc}

In last [post](https://leezhenghui.github.io/microservices/2018/11/01/build-a-scalable-system-practice-on-gateway-mode-for-mixed-lang.html), I introduced the gateway mode via a PoC sample containing java and node.js services.  This post will focus on the PoC sample for service mesh mode, which can provide a solution for a mixed languages development challenges in MSA.

All of the sample source code is hosted on [repsoitry](https://github.com/leezhenghui/hello-servicemesh.git), it is automated, and easy to run. To simplify the sample content, I just keep the gateway-mode related features in it, if you are interested in aggregated logging, performance analysis, please refer to earlier posts. 

## Recall the service mesh mode

<img src="{{ site.url }}/assets/materials/build-scalable-system/architecture-servicemesh-mode.png" alt="leaky-bucket-algorithm.jpeg">

Please refer to my early post [reference-modes-for-msa-service-communication](https://leezhenghui.github.io/microservices/2018/10/20/build-a-scalable-system-runtime-challenges.html#heading-reference-modes-for-msa-service-communication) for more details about the MSA runtime challenges.

## PoC sample

### Scenario overview

<img src="{{ site.url }}/assets/materials/build-scalable-system/architecture-servicemesh-components.png" alt="architecture-servicemesh-components.png">

### Sidecar Tasks 

<img src="{{ site.url }}/assets/materials/build-scalable-system/architecture-sidecar-proxy-with-taskgroup.png" alt="architecture-servicemesh-components.png">

>
> The sample is based on `consul connect` and it's builtin proxy for the serviemesh. We want to use it to demonsrate the service mesh concepts for a hybrid environments(docker is not the only packing method, mixed deployable types for the service orchestration which could not be achieved by k8s only. cloud-native and on-premise co-exists). Please note, Consule connect was introduced in Consul 1.2 and be marked as beta quality for now, thus, not ready to be used in a production environment.


### Soruce code structure 

#### Modules
```

├── modules
│   ├── add.svc               // add operator service, which will be called by calculator-ui
│   │   ├── build.gradle
│   │   ├── out
│   │   └── src
│   ├── api.gateway           // api.gateway based on Zuul
│   │   ├── build.gradle
│   │   ├── out
│   │   └── src
│   ├── frontend             // front-end service(edge service), which is implemeted by node.js, will call to add.svc and sub.svc 
│   │   ├── build.gradle
│   │   ├── out
│   │   └── src
│   ├── ifw.lib               // A prototype simple library impl to demonstrate an AOP based invocation framework with annotated QoS supports (just for demo only, not a production quality)
│   │   ├── build.gradle
│   │   ├── out
│   │   └── src
│   ├── sub.svc               // subtract operator service, which will be called by calculator-ui
│   │   ├── build.gradle
│   │   ├── out
│   │   └── src

```

#### Operational source code 

```
ops
├── Vagrantfile       // Vagrant file
├── ansible           // ansible scripts for install and start services, including: commoent runtime dependences, zookeeper, kafka, nginx(for local pkgs repo), install JVM, filebeat, consul, nomad, elasticsearch, logstash, kibana and wrk
├── bin               // script, including boostrap.sh, click.sh(fire an invocation on the sample), kafka-*-monitor.sh, start_all_jobs.sh and stop_all_jobs.sh
├── deployable        // nomad job definition files(hcl) for microservices
├── deps              // binary dependences, which cache it locally to reduce(avoid) network deps during demonstration
├── dist              // pkgs publish folder, nginx is started on this folder to simulate a pkg repository
```

## Run the Sample 

All of service instances are using dynamic ports in this sample to demonstrate the auto-scale features. 

> 
> As we want to focus on servicemesh part in the sample, to make the verification(for service discovery and load balancing) ealier, only the internal services are scheduled multiple instances in this sample. If you are interested in the edge services, please refer to [hello-msaproxy](https://github.com/leezhenghui/hello-msaproxy) for details.

### Steps

- Prerequisites 
  - Java
  - Node.js
  - Gradle

- Git clone the project

	On host:
  ```shell
  git clone git@github.com:leezhenghui/hello-servicemesh.git
  ```

- Gradle build/deploy distribution
  
  On host:
  ```shell
  cd hello-servicemesh/modules/frontend
  npm install
  cd ../../
  gradle deploy 
  ```

- Launch VM 
  
	On host:
	```shell
	cd ops
	vagrant up
  ```

- Provision the VM 
  
	On host:
	```shell
	vagrant provision 
  ```

- Start all nomad jobs 

  For each services, two intances will be created for a load balance, service discovery testing
  
	On host:

	```shell
  vagrant ssh
  ```

	In VM
	```shell
	cd /vagrant/bin
	./start_all_jobs.sh
	```

  After start all jobs, we can see 4 connect-proxy(inbound side proxy) instances created:
	```shell
	root      4222  4180  0 14:25 ?        00:00:00 /opt/consul/bin/consul connect proxy -service add-svc -service-addr 10.10.10.150:22130 -listen :28489 -register -register-id 28489
	root      4223  4171  0 14:25 ?        00:00:00 /opt/consul/bin/consul connect proxy -service add-svc -service-addr 10.10.10.150:24291 -listen :26604 -register -register-id 26604
	root      4273  4203  0 14:25 ?        00:00:00 /opt/consul/bin/consul connect proxy -service sub-svc -service-addr 10.10.10.150:23454 -listen :26351 -register -register-id 26351
	root      4275  4207  0 14:25 ?        00:00:00 /opt/consul/bin/consul connect proxy -service sub-svc -service-addr 10.10.10.150:25384 -listen :28277 -register -register-id 28277
	```

	as well as two upstream-proxy(outbound side proxy) instances:
	```shell
	root      4304  4259  0 14:25 ?        00:00:00 /opt/consul/bin/consul connect proxy -service frontend -upstream sub-svc:28153
	root      4322  4260  0 14:25 ?        00:00:00 /opt/consul/bin/consul connect proxy -service frontend -upstream add-svc:28663
	```

- Run the sample 

  In VM:
	```shell
	./click.sh
  ```

- Run benchmark 

  In VM:
	```shell
	./benchmark.sh
  ```

### Result of Service Discovery and Load Balance

<img src="{{ site.url }}/assets/materials/build-scalable-system/lb-result-servicemesh.png" alt="architecture-servicemesh-components.png">


## Wrapping up

In this post, we introduce the service mesh mode via PoC sample, in next post, we will take a hands-on practice on service mesh mode with `envoy proxy` for a mixed programming language scenario.

{% include common/series.html %}
