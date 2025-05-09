'use client';

import { useState, useEffect } from 'react';
import { useRouter } from 'next/navigation';
import Image from 'next/image';
import { Card, CardContent, CardDescription, CardFooter, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Alert } from '@/components/ui/alert';
import { setClientAuthCookie, getClientAuthStatus } from '@/lib/auth';

export default function LoginPage() {
  const router = useRouter();
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(false);
  const [redirecting, setRedirecting] = useState(false);

  // Check for existing authentication on load
  useEffect(() => {
    // If already authenticated, redirect to dashboard
    if (getClientAuthStatus()) {
      console.log('User already authenticated, redirecting to dashboard');
      router.push('/dashboard');
    }
  }, [router]);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!password) {
      setError('Please enter the password');
      return;
    }

    setLoading(true);
    setError('');

    try {
      console.log('Attempting login...');
      const response = await fetch('/api/auth/login', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ password }),
      });

      const data = await response.json();
      console.log('Login response status:', response.status);

      if (response.ok) {
        console.log('Authentication successful, setting cookie and redirecting');
        // Store the token in a cookie
        setClientAuthCookie(data.token);
        
        // Set redirecting state to show feedback
        setRedirecting(true);
        
        // Give the cookie a moment to be set before redirecting
        setTimeout(() => {
          // Force a hard navigation to dashboard instead of client-side routing
          window.location.href = '/dashboard';
        }, 500);
      } else {
        console.error('Authentication failed:', data.message);
        setError(data.message || 'Authentication failed');
      }
    } catch (err) {
      console.error('Login error:', err);
      setError('An error occurred. Please try again.');
    } finally {
      if (!redirecting) {
        setLoading(false);
      }
    }
  };

  return (
    <div className="min-h-screen flex flex-col items-center justify-center p-4 bg-background">
      <div className="w-full max-w-md">
        <div className="text-center mb-8">
          <div className="flex justify-center mb-6">
            <div className="relative w-24 h-24 coffee-steam">
              <Image 
                src="/images/coffee-cup.svg" 
                alt="Homebrew Logo" 
                width={96} 
                height={96}
                className="rounded-full bg-secondary p-4"
              />
            </div>
          </div>
          <h1 className="text-4xl font-bold text-primary">Homebrew Gandalf</h1>
          <p className="text-xl mt-2 text-muted-foreground">You shall not pass... without authentication!</p>
        </div>

        <Card className="border-border shadow-lg">
          <CardHeader>
            <CardTitle className="text-2xl">Authentication Required</CardTitle>
            <CardDescription>
              Enter the password to access the door control system
            </CardDescription>
          </CardHeader>
          <CardContent>
            <form onSubmit={handleSubmit}>
              <div className="space-y-4">
                <div className="space-y-2">
                  <Input
                    id="password"
                    type="password"
                    placeholder="Enter password"
                    value={password}
                    onChange={(e) => setPassword(e.target.value)}
                    className="w-full"
                    autoComplete="current-password"
                    disabled={loading || redirecting}
                  />
                </div>
                
                {error && (
                  <Alert variant="destructive" className="text-destructive bg-destructive/10 py-2">
                    {error}
                  </Alert>
                )}
                
                <Button 
                  type="submit" 
                  className="w-full bg-primary hover:bg-primary/90" 
                  disabled={loading || redirecting}
                >
                  {loading ? (
                    <span className="flex items-center justify-center">
                      <span className="w-5 h-5 border-2 border-white border-t-transparent rounded-full animate-spin mr-2" /> 
                      Authenticating...
                    </span>
                  ) : redirecting ? (
                    <span className="flex items-center justify-center">
                      <span className="w-5 h-5 border-2 border-white border-t-transparent rounded-full animate-spin mr-2" /> 
                      Redirecting to dashboard...
                    </span>
                  ) : (
                    'Authenticate'
                  )}
                </Button>
              </div>
            </form>
          </CardContent>
          <CardFooter className="flex justify-center text-sm text-muted-foreground">
            Contact your administrator if you need assistance
          </CardFooter>
        </Card>
      </div>
    </div>
  );
} 