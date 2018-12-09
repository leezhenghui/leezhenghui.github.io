---
layout: post
title: Build a Modern Scalable System - Practice on Gateway Mode For Mixed-Languages Case 
categories: [microservices]
tags: [architecture, microservices, java, gateway]
series: [build-scalable-system]
comments: true
---

{% include common/series.html %}

## Table of Contents

* Kramdown table of contents
{:toc .toc}

In last [post](https://leezhenghui.github.io/microservices/2018/10/27/build-a-scalable-system-practice-on-springcloud.html), I introduced the embedded router mode via spring-cloud PoC sample.  This post will focus on the PoC sample for gateway mode, which can provide a solution for a mixed languages development challenges in MSA.

All of the sample source code is hosted on [repsoitry](https://github.com/leezhenghui/hello-msaproxy.git), it is automated, and easy to run. To simplify the sample content, I just keep the gateway-mode related features in it, if you are interested in aggregated logging, performance analysis, please refer to earlier posts. 

## Recall the gateway mode

<img src="{{ site.url }}/assets/materials/build-scalable-system/architecture-gateway-mode.png" alt="leaky-bucket-algorithm.jpeg">

Please refer to my early post [reference-modes-for-msa-service-communication](https://leezhenghui.github.io/microservices/2018/10/20/build-a-scalable-system-runtime-challenges.html#heading-reference-modes-for-msa-service-communication) for more details about the MSA runtime challenges.

## PoC sample

### Scenario overview

<img src="{{ site.url }}/assets/materials/build-scalable-system/architecture-msaproxy-components.png" alt="architecture-msaproxy-components.png">

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
└── svc-ref-register  // internal domain name registry by service short name 
```

## Load Balancer upstream dynamic update

- Nginx/HAProxy with Consul Template

- Nginx with Custom Module
  > - [Dynamic Nginx Upstreams for Consul via lua-nginx-module by Zachary Schneider](https://medium.com/@sigil66/dynamic-nginx-upstreams-from-consul-via-lua-nginx-module-2bebc935989b)
  > - [Nginx upsync module](https://github.com/weibocom/nginx-upsync-module)
  > - [Nginx Pro DNS resolution](https://www.nginx.com/blog/service-discovery-nginx-plus-srv-records-consul-dns/)
  > - [ngx\_http\_set\_backend](https://github.com/robinmonjo/ngx_http_l) which inspired binding Nginx modules in C to Consul in Golang

- HAProxy 1.8

  > HAProxy 1.8 brings resolution for ports through SRV records and support for EDNS, making it pair perfectly with Consul.

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
  git clone git@github.com:leezhenghui/hello-msaproxy.git
  ```

- Gradle build/deploy distribution
  
  On host:
  ```shell
  cd hello-msaproxy/modules/frontend
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

<img src="{{ site.url }}/assets/materials/build-scalable-system/lb-result-gateway-poc-sample.png" alt="lb-result-gateway-poc-sample.png">

## Wrapping up

In this post, we introduce the gateway mode via PoC sample, in next post, we will take a hands-on practice on service mesh mode for a mixed programming language scenario.

{% include common/series.html %}
