import React, { useState } from 'react';
import { Icon } from '../common/Icon';
import { Button } from '../common/Button';
import { Input } from '../common/Input';
// import './RecoverySetup.scss';

interface RecoveryQuestion {
  id: string;
  text: string;
  answer: string;
}

const DEFAULT_QUESTIONS = [
  "What was the name of your first pet?",
  "What city were you born in?",
  "What was your mother's maiden name?",
  "What was the name of your first school?",
  "What was your childhood nickname?",
  "What was the make of your first car?",
  "What was the name of your favorite teacher?",
  "What was your first job?",
  "What was the name of the street you grew up on?",
  "What was your favorite book as a child?"
];

interface RecoverySetupProps {
  vaultId: string;
  onSetupComplete: (recoveryInfo: any) => void;
  onSetupCancel: () => void;
  isLoading?: boolean;
}

export const RecoverySetup: React.FC<RecoverySetupProps> = ({
  vaultId,
  onSetupComplete,
  onSetupCancel,
  isLoading = false,
}) => {
  const [questions, setQuestions] = useState<RecoveryQuestion[]>([
    { id: '1', text: '', answer: '' },
    { id: '2', text: '', answer: '' },
    { id: '3', text: '', answer: '' }
  ]);
  const [selectedQuestionIndex, setSelectedQuestionIndex] = useState<number | null>(null);
  const [customQuestion, setCustomQuestion] = useState('');
  const [errors, setErrors] = useState<string[]>([]);

  const handleQuestionChange = (index: number, value: string) => {
    const newQuestions = [...questions];
    newQuestions[index].text = value;
    setQuestions(newQuestions);
    setErrors([]);
  };

  const handleAnswerChange = (index: number, value: string) => {
    const newQuestions = [...questions];
    newQuestions[index].answer = value;
    setQuestions(newQuestions);
    setErrors([]);
  };

  const handleSelectDefaultQuestion = (questionText: string) => {
    if (selectedQuestionIndex !== null) {
      const newQuestions = [...questions];
      newQuestions[selectedQuestionIndex].text = questionText;
      setQuestions(newQuestions);
      setSelectedQuestionIndex(null);
      setCustomQuestion('');
    }
  };

  const handleAddCustomQuestion = () => {
    if (customQuestion.trim() && selectedQuestionIndex !== null) {
      const newQuestions = [...questions];
      newQuestions[selectedQuestionIndex].text = customQuestion.trim();
      setQuestions(newQuestions);
      setSelectedQuestionIndex(null);
      setCustomQuestion('');
    }
  };

  const handleRemoveQuestion = (index: number) => {
    if (questions.length > 1) {
      const newQuestions = questions.filter((_, i) => i !== index);
      setQuestions(newQuestions);
    }
  };

  const handleAddQuestion = () => {
    if (questions.length < 5) {
      const newQuestions = [...questions, { id: Date.now().toString(), text: '', answer: '' }];
      setQuestions(newQuestions);
    }
  };

  const validateForm = (): boolean => {
    const newErrors: string[] = [];

    questions.forEach((question, index) => {
      if (!question.text.trim()) {
        newErrors.push(`Question ${index + 1} is required`);
      }
      if (!question.answer.trim()) {
        newErrors.push(`Answer for question ${index + 1} is required`);
      }
      if (question.answer.trim().length < 3) {
        newErrors.push(`Answer for question ${index + 1} must be at least 3 characters`);
      }
    });

    // Check for duplicate questions
    const questionTexts = questions.map(q => q.text.toLowerCase().trim());
    const uniqueQuestions = new Set(questionTexts);
    if (uniqueQuestions.size !== questionTexts.length) {
      newErrors.push('All questions must be unique');
    }

    setErrors(newErrors);
    return newErrors.length === 0;
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();

    if (!validateForm()) {
      return;
    }

    try {
      // TODO: Call the actual recovery setup API
      // const recoveryInfo = await setupPasswordRecovery(vaultId, questions);
      // onSetupComplete(recoveryInfo);
      
      // Mock success for demonstration
      const mockRecoveryInfo = {
        vaultId,
        questions: questions.map(q => ({
          question_id: q.id,
          question_text: q.text,
          answer: q.answer
        }))
      };
      
      onSetupComplete(mockRecoveryInfo);
    } catch (err) {
      setErrors([err instanceof Error ? err.message : 'Setup failed']);
    }
  };

  const handleCancel = () => {
    onSetupCancel();
  };

  return (
    <div className="recovery-setup">
      <div className="recovery-setup__header">
        <Icon name="shield-plus" className="recovery-setup__icon" />
        <h2 className="recovery-setup__title">Set Up Password Recovery</h2>
        <p className="recovery-setup__subtitle">
          Create security questions to recover access if you forget your password
        </p>
      </div>

      <form className="recovery-setup__form" onSubmit={handleSubmit}>
        <div className="recovery-setup__questions">
          <h3 className="recovery-setup__section-title">Security Questions</h3>
          <p className="recovery-setup__section-description">
            Choose 3-5 security questions and provide answers. These will be used to recover your vault if you forget your password.
          </p>

          {questions.map((question, index) => (
            <div key={question.id} className="recovery-setup__question">
              <div className="recovery-setup__question-header">
                <label className="recovery-setup__question-label">
                  Question {index + 1}
                </label>
                {questions.length > 1 && (
                  <Button
                    type="button"
                    variant="ghost"
                    size="sm"
                    onClick={() => handleRemoveQuestion(index)}
                    className="recovery-setup__remove-question"
                  >
                    <Icon name="trash" />
                  </Button>
                )}
              </div>

              <div className="recovery-setup__question-input">
                <Input
                  type="text"
                  value={question.text}
                  onChange={(e) => handleQuestionChange(index, e.target.value)}
                  placeholder="Enter your security question"
                  className="recovery-setup__question-text"
                  disabled={isLoading}
                />
                <Button
                  type="button"
                  variant="outline"
                  size="sm"
                  onClick={() => setSelectedQuestionIndex(index)}
                  className="recovery-setup__select-question"
                >
                  <Icon name="list" />
                  Choose from list
                </Button>
              </div>

              <Input
                type="text"
                value={question.answer}
                onChange={(e) => handleAnswerChange(index, e.target.value)}
                placeholder="Enter your answer"
                className="recovery-setup__answer-input"
                disabled={isLoading}
              />
            </div>
          ))}

          {questions.length < 5 && (
            <Button
              type="button"
              variant="outline"
              onClick={handleAddQuestion}
              className="recovery-setup__add-question"
              disabled={isLoading}
            >
              <Icon name="plus" />
              Add Another Question
            </Button>
          )}
        </div>

        {errors.length > 0 && (
          <div className="recovery-setup__errors">
            <Icon name="error" />
            <ul>
              {errors.map((error, index) => (
                <li key={index}>{error}</li>
              ))}
            </ul>
          </div>
        )}

        <div className="recovery-setup__actions">
          <Button
            type="button"
            variant="outline"
            onClick={handleCancel}
            disabled={isLoading}
            className="recovery-setup__cancel"
          >
            <Icon name="close" />
            Cancel
          </Button>
          <Button
            type="submit"
            variant="primary"
            disabled={isLoading}
            className="recovery-setup__submit"
          >
            {isLoading ? (
              <>
                <Icon name="loading" className="recovery-setup__loading" />
                Setting up...
              </>
            ) : (
              <>
                <Icon name="check" />
                Set Up Recovery
              </>
            )}
          </Button>
        </div>
      </form>

      {/* Question Selection Modal */}
      {selectedQuestionIndex !== null && (
        <div className="recovery-setup__modal">
          <div className="recovery-setup__modal-content">
            <div className="recovery-setup__modal-header">
              <h3>Choose a Question</h3>
              <Button
                type="button"
                variant="ghost"
                onClick={() => setSelectedQuestionIndex(null)}
              >
                <Icon name="close" />
              </Button>
            </div>
            <div className="recovery-setup__modal-body">
              <div className="recovery-setup__default-questions">
                {DEFAULT_QUESTIONS.map((question, index) => (
                  <button
                    key={index}
                    type="button"
                    className="recovery-setup__default-question"
                    onClick={() => handleSelectDefaultQuestion(question)}
                  >
                    {question}
                  </button>
                ))}
              </div>
              <div className="recovery-setup__custom-question">
                <Input
                  type="text"
                  value={customQuestion}
                  onChange={(e) => setCustomQuestion(e.target.value)}
                  placeholder="Or enter a custom question"
                />
                <Button
                  type="button"
                  variant="primary"
                  onClick={handleAddCustomQuestion}
                  disabled={!customQuestion.trim()}
                >
                  Use Custom Question
                </Button>
              </div>
            </div>
          </div>
        </div>
      )}

      <div className="recovery-setup__help">
        <Icon name="info" />
        <div className="recovery-setup__help-content">
          <h4>Recovery Tips:</h4>
          <ul>
            <li>Choose questions only you can answer</li>
            <li>Use answers that won't change over time</li>
            <li>Make answers memorable but not easily guessable</li>
            <li>Answers are case-sensitive</li>
          </ul>
        </div>
      </div>
    </div>
  );
};
