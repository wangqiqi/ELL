/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Machine Learning Library (EMLL)
//  File:     Nodes_test.h (nodes_test)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

void TestL2NormNodeCompute();
void TestAccumulatorNodeCompute();
void TestDelayNodeCompute();
void TestMovingAverageNodeCompute();
void TestMovingVarianceNodeCompute();
void TestUnaryOperationNodeCompute();
void TestBinaryOperationNodeCompute();
void TestLinearPredictorNodeCompute();
void TestDemultiplexerNodeCompute();

// Refinement
void TestMovingAverageNodeRefine();
void TestLinearPredictorNodeRefine();
void TestSimpleForestNodeRefine();
void TestDemultiplexerNodeRefine();
