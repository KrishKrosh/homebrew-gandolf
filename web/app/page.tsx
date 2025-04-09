'use client';

import { useEffect } from 'react';
import { useRouter } from 'next/navigation';
import { getClientAuthStatus } from '@/lib/auth';

export default function HomePage() {
  const router = useRouter();

  useEffect(() => {
    // Check if user is authenticated and redirect accordingly
    if (getClientAuthStatus()) {
      router.push('/dashboard');
    } else {
      router.push('/login');
    }
  }, [router]);

  // Return a loading state while redirecting
  return (
    <div className="min-h-screen flex items-center justify-center bg-background">
      <div className="text-center">
        <h1 className="text-2xl font-bold mb-4">Homebrew Gandalf</h1>
        <p className="text-muted-foreground mb-4">Loading, please wait...</p>
        <div className="w-10 h-10 border-4 border-primary border-t-transparent rounded-full animate-spin mx-auto"></div>
      </div>
    </div>
  );
}
