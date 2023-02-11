---
layout: post
title:  "A Simple Painting Style Classifier"
date:   2023-02-13 15:12:04 +0100
categories: jekyll update
---

As a first project for my portfolio, I decided to build a simple deep learning model which can classify paintings based on their styles. As of now, the model supports only binary classification - impressionism and cubism. This project involved the whole end-to-end process - data collection, data cleaning and transformation, model training and testing, and finally deploying it as a web application.

## Motivation

The motivation for this project came from [an online course](https://www.w3schools.io/file/markdown-links/) on deep learning which I'm currently following (particularly Chapters 1 and 2). In it, the instructor shows how to build an image classifier for a simple task - identifying whether the given image is of a dog vs a cat. He then shows how to host the model on a server and deploy it using a basic web application. I decided to adapt the process of my personal interest. I enjoy visiting art museums and looking at paintings. I thought I'd build a painting style identifier. I initially started with only two types - impressionism and cubism, because the two styles have such strong characteristics and hence are easy to distinguish by humans.

## Data Collection and Processing

I began the project on Google Colab using Jupyter notebook. Colab provides free GPU for our model training and Jupyter notebook is helpful for prototyping and quickly visualising results. I gathered image data using DuckDuckGo search API. I've read that Microsoft Bing API is better in terms of providing more relevant images, but needs some configuring intially. Due to this, I decided to postpone usage of Bing to a later iteration. I collected 90 images of each class - a small dataset because instead of training an image classifier from scratch, I fine-tuned a powerful model trained on a large dataset. 

There is some preprocessing to be done before we can use our image data:

1. Image resizing - The images we've scrapped are of different sizes. However, our deep learning model exects all images in the dataset to have the same size. Image resizing can be done in multiple ways. Here I chose random cropping. On each epoch (which is one complete pass through all of our images in the dataset) we randomly select a randomly selected 224x224 patch of each image. This method has the advantage that overall, we use information from all parts of the image and do not arbitrarily distort the images. This process can also be looked as a way to augment the dataset since each image can yield muliple cropped samples.

2. Data augmentation - To increase the size of the dataset and to make our model our robust, each image can be flipped, rotated, warped and have brightness and contrast changed.

Both of these above functionalities are provided by the fastai library. fastai is a high-level Python library for deep learning built on top of PyTorch that makes the whole process of data cleaning, training and testing easier and faster. 

## Model Training

I bypassed image cleaning and directly decided to train a deep learning model. I used a ResNet model with 34 layers, pretrained on the ImageNet dataset to perform image classification. This model is provided for use via fastai. Since the model is already pre-trained, I only had to fine-tune it for my style classification task. Using a pre-trained model meant that I did not need a huge amount of training data or computing resource/time.

fastai provides a very useful function that suggests a good learning rate to use for training. I fined-tuned the model with the suggested learning-rate.

## Model Validation and Training

The error on the validation set during training goes down to zero completely. I looked at some of the outputs from the model and saw that they're correctly classified.

I also tested the model on a couple of hand-picked images and visualized the responses.

## Deployment as a Web Application

I exported and saved the model from the Colab notebook. I used Gradio to demo my model as a web app. It wraps a python function into a user interface and allows us to launch the demos inside jupyter notebooks. The model was hosted on Hugging Face Spaces.

Finally to create the web application, I used javascript that accepts the uploaded image, calls the gradio function for inference, and displays the returned results.

## Further Work

I tried extending this model to identify more types of painting styles. I'm not an art expert, so after some online research I decided to use 9 different painting styles which are considered important. I ran into the problem that the final model was very inaccurate, no matter how much I tried improving training.

A visualisation of the images where the model performed poorly revealed that some of the images scrapped via the DuckDuckGo API were irrelevant and had to be removed. fastai provides a way for us to remove these irrelevant data via a GUI interface. I did this and still the accuracy was poor. I began to suspect that the images returned in response to search queries didn't actually belong to the right styles. All of this suggested that I needed a better way to build up a cleaner and more accurate dataset. However, this process is going to be time-consuming and hence I decided to postpone it for a later iteration.


