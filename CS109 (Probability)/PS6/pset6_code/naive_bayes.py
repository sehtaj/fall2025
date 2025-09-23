'''
Naive Bayes Classifier
'''
import math
import numpy as np

"""
Starter Code for CS 109 Problem Set 6
Written by Tim Gianitsos

*************************IMPORTANT*************************
Do NOT modify the name of any functions. Do not add or remove
parameters to them either. Moreover, make sure your return
value is exactly as described in the PDF handout and in the
provided function comments. Remember that your code is being
autograded. You are free to write helper functions if you so
desire but they are not necessary. Do NOT rename this file.
Do NOT modify any code outside the begin and end code markers.
*************************IMPORTANT*************************
"""

class NaiveBayes:
    '''
    Naive Bayes Classifier

    For a datapoint, the Naive Bayes classifier computes the probability of each label,
    and then it predicts the label with the highest probability. During training,
    it learns probabilities by counting the occurences of feature/label combinations
    that it finds in the training data. During prediction, it uses these counts to
    compute probabilities.
    '''

    def __init__(self, use_mle):
        '''DO NOT RENAME INSTANCE VARIABLES'''
        self.label_counts = {}
        self.feature_counts = {}
        self.use_mle = use_mle # True for MLE, False for MAP with Laplace add-one smoothing

    def fit(self, train_features, train_labels):
        self.label_counts[0] = 0
        self.label_counts[1] = 0

        ### YOUR CODE HERE (~5-10 Lines)
        n_samples, n_features = train_features.shape

        for i in range(n_samples):
            label = train_labels[i]
            self.label_counts[label] += 1

            for f in range(n_features):
                val = train_features[i, f]
                key = (f, val, label)
                self.feature_counts[key] = self.feature_counts.get(key, 0) + 1

        ### END YOUR CODE

    def predict(self, test_features):
        preds = np.zeros(test_features.shape[0], dtype=np.uint8)

        ### YOUR CODE HERE (~10-30 Lines)
        n_samples, n_features = test_features.shape

        for i in range(n_samples):
            sample = test_features[i]

            prob_0 = self.label_counts[0] / sum(self.label_counts.values())
            prob_1 = self.label_counts[1] / sum(self.label_counts.values())

            for f in range(n_features):
                val = sample[f]
                key0 = (f, val, 0)
                key1 = (f, val, 1)

                count0 = self.feature_counts.get(key0, 0)
                count1 = self.feature_counts.get(key1, 0)
        if self.use_mle:
            if self.label_counts[0] > 0:
                total0 = self.label_counts[0]
            else:
                total0 = 1
            if self.label_counts[1] > 0:
                total1 = self.label_counts[1]
            else:
                total1 = 1

            prob_0 = prob_0 * (count0 / total0)
            prob_1 = prob_1 * (count1 / total1)

        else:
            total0 = self.label_counts[0] + 2
            total1 = self.label_counts[1] + 2

            prob_0 = prob_0 * ((count0 + 1) / total0)
            prob_1 = prob_1 * ((count1 + 1) / total1)

            if prob_0 >= prob_1:
                preds[i] = 0
            else:
                preds[i] = 1

        ### END YOUR CODE

        return preds
