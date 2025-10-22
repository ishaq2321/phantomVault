import React, { useState, useEffect } from 'react';
import { Icon } from '../common/Icon';
import { Button } from '../common/Button';
import { Input } from '../common/Input';
// import './PasswordRecovery.scss';

interface RecoveryQuestion {
  question_id: string;
  question_text: string;
}

interface PasswordRecoveryProps {
  vaultId: string;
  questions: RecoveryQuestion[];
  onRecoverySuccess: (recoveryKey: string) => void;
  onRecoveryCancel: () => void;
  isLoading?: boolean;
  error?: string;
}

export const PasswordRecovery: React.FC<PasswordRecoveryProps> = ({
  vaultId,
  questions,
  onRecoverySuccess,
  onRecoveryCancel,
  isLoading = false,
  error,
}) => {
  const [answers, setAnswers] = useState<string[]>(new Array(questions.length).fill(''));
  const [attemptsRemaining, setAttemptsRemaining] = useState<number>(3);
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [submitError, setSubmitError] = useState<string>('');

  const handleAnswerChange = (index: number, value: string) => {
    const newAnswers = [...answers];
    newAnswers[index] = value;
    setAnswers(newAnswers);
    setSubmitError('');
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    
    if (answers.some(answer => answer.trim() === '')) {
      setSubmitError('Please answer all questions');
      return;
    }

    setIsSubmitting(true);
    setSubmitError('');

    try {
      // TODO: Call the actual recovery verification API
      // const recoveryKey = await verifyRecoveryAnswers(vaultId, answers);
      // onRecoverySuccess(recoveryKey);
      
      // Simulate API call for now
      await new Promise(resolve => setTimeout(resolve, 2000));
      
      // Mock success for demonstration
      onRecoverySuccess('mock-recovery-key');
    } catch (err) {
      setSubmitError(err instanceof Error ? err.message : 'Recovery failed');
      setAttemptsRemaining(prev => Math.max(0, prev - 1));
    } finally {
      setIsSubmitting(false);
    }
  };

  const handleCancel = () => {
    onRecoveryCancel();
  };

  return (
    <div className="password-recovery">
      <div className="password-recovery__header">
        <Icon name="shield-question" className="password-recovery__icon" />
        <h2 className="password-recovery__title">Password Recovery</h2>
        <p className="password-recovery__subtitle">
          Answer your security questions to recover access to your vault
        </p>
      </div>

      <div className="password-recovery__status">
        <div className="password-recovery__attempts">
          <Icon name="key" />
          <span>Attempts remaining: {attemptsRemaining}</span>
        </div>
        {attemptsRemaining <= 1 && (
          <div className="password-recovery__warning">
            <Icon name="warning" />
            <span>Warning: Only {attemptsRemaining} attempt(s) remaining</span>
          </div>
        )}
      </div>

      <form className="password-recovery__form" onSubmit={handleSubmit}>
        <div className="password-recovery__questions">
          {questions.map((question, index) => (
            <div key={question.question_id} className="password-recovery__question">
              <label className="password-recovery__question-label">
                {index + 1}. {question.question_text}
              </label>
              <Input
                type="text"
                value={answers[index]}
                onChange={(e) => handleAnswerChange(index, e.target.value)}
                placeholder="Enter your answer"
                className="password-recovery__answer-input"
                disabled={isSubmitting || isLoading}
                required
              />
            </div>
          ))}
        </div>

        {(error || submitError) && (
          <div className="password-recovery__error">
            <Icon name="error" />
            <span>{error || submitError}</span>
          </div>
        )}

        <div className="password-recovery__actions">
          <Button
            type="button"
            variant="outline"
            onClick={handleCancel}
            disabled={isSubmitting || isLoading}
            className="password-recovery__cancel"
          >
            <Icon name="close" />
            Cancel
          </Button>
          <Button
            type="submit"
            variant="primary"
            disabled={isSubmitting || isLoading || answers.some(answer => answer.trim() === '')}
            className="password-recovery__submit"
          >
            {isSubmitting ? (
              <>
                <Icon name="loading" className="password-recovery__loading" />
                Verifying...
              </>
            ) : (
              <>
                <Icon name="check" />
                Verify Answers
              </>
            )}
          </Button>
        </div>
      </form>

      <div className="password-recovery__help">
        <Icon name="info" />
        <div className="password-recovery__help-content">
          <h4>Recovery Tips:</h4>
          <ul>
            <li>Answers are case-sensitive</li>
            <li>Use the exact answers you provided during setup</li>
            <li>If you can't remember an answer, try variations you might have used</li>
            <li>Contact support if you've exhausted all attempts</li>
          </ul>
        </div>
      </div>
    </div>
  );
};
