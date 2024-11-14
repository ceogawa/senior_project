#pragma once
#ifndef __trans__
#define __trans__

class transForms {
public:
	//model transforms
	glm::vec3 mTrans;
	float mRot;
	float mScale;

	//model bounding sphere radius
	float mRadius;
	
	//model draw attributes
	int matID;
	transForms(glm::vec3 inTrans, float inRot, float inS, float inR, int inMatID): 
		mTrans(inTrans), mRot(inRot), mScale(inS), mRadius(inR), matID(inMatID) {}
  
  };

#endif